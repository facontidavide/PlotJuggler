#include "websocket_client.h"

#include <QSettings>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>
#include <QIntValidator>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTreeWidget>
#include <QtEndian>
#include <cstring>

#include <zstd.h>

#include <algorithm>
#include <set>

#include "ui_websocket_client.h"

// =======================
// Connection dialog
// =======================
class WebsocketDialog : public QDialog
{
public:
  WebsocketDialog() : QDialog(nullptr), ui(new Ui::WebSocketDialog)
  {
    // Build UI
    ui->setupUi(this);

    // Window title
    setWindowTitle("WebSocket Client");

    // Allow only valid TCP ports
    ui->lineEditPort->setValidator(new QIntValidator(1, 65535, this));

    ui->comboBox->setEnabled(false);
  }

  ~WebsocketDialog() { delete ui; }

public:
  Ui::WebSocketDialog* ui;
};

// =======================
// WebsocketClient
// =======================
WebsocketClient::WebsocketClient() : _running(false), _dialog(nullptr)
{
  // Initial state
  _state.mode = WsState::Mode::Close;
  _state.req_in_flight = false;

  // Pending request tracking
  _pendingRequestId.clear();
  _pendingMode = WsState::Mode::Close;

  // Timer used to periodically request topics (only while selecting topics)
  _topicsTimer.setInterval(1000);
  connect(&_topicsTimer, &QTimer::timeout, this, &WebsocketClient::requestTopics);

  // Heartbeat timer (used in Data mode)
  _heartBeatTimer.setInterval(1000);
  connect(&_heartBeatTimer, &QTimer::timeout, this, &WebsocketClient::sendHeartBeat);

  // WebSocket signals
  connect(&_socket, &QWebSocket::connected, this, &WebsocketClient::onConnected);
  connect(&_socket, &QWebSocket::textMessageReceived, this, &WebsocketClient::onTextMessageReceived);
  connect(&_socket, &QWebSocket::binaryMessageReceived, this, &WebsocketClient::onBinaryMessageReceived);
  connect(&_socket, &QWebSocket::disconnected, this, &WebsocketClient::onDisconnected);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  connect(&_socket, &QWebSocket::errorOccurred, this, &WebsocketClient::onError);
#else
  connect(&_socket,
          QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
          this,
          &WebsocketClient::onError);
#endif
}

// =======================
// Filter helpers
// =======================
static void applyTopicFilterKeepSelected(QTreeWidget* list, const QString& filter)
{
  if (!list) return;

  const QString fl = filter.trimmed().toLower();

  for (int i = 0; i < list->topLevelItemCount(); i++) {
    auto* it = list->topLevelItem(i);

    const bool selected = it->isSelected();

    if (fl.isEmpty()) {
      it->setHidden(false);
      continue;
    }

    const QString name = it->text(0).toLower();
    const QString type = it->text(1).toLower();
    const bool match = name.contains(fl) || type.contains(fl);

    it->setHidden(!(match || selected));
  }
}

// =======================
// Start client
// =======================
bool WebsocketClient::start(QStringList*)
{
  // Already running
  if (_running) return true;

  // Load stored port (default 8080)
  QSettings settings;
  QString address = settings.value("WebsocketClient::address", "127.0.0.1").toString();
  int port = settings.value("WebsocketClient::port", 8080).toInt();

  // Create dialog (stack object)
  WebsocketDialog dialog;
  _dialog = &dialog;

  dialog.ui->lineEditAddress->setText(address);
  dialog.ui->lineEditPort->setText(QString::number(port));

  // Rename OK button (will toggle between Connect/Subscribe)
  auto okBtn = dialog.ui->buttonBox->button(QDialogButtonBox::Ok);
  if (okBtn) okBtn->setText("Connect");

  // Enable/disable OK button depending on state
  auto refreshOk = [&]() {
    if (!dialog.ui->buttonBox) return;
    auto b = dialog.ui->buttonBox->button(QDialogButtonBox::Ok);
    if (!b) return;

    // Not connected yet: allow connect
    if (!_running) {
      b->setEnabled(true);
      b->setText("Connect");
      return;
    }

    // Connected and waiting for topic selection: allow subscribe when something is selected
    if (_state.mode == WsState::Mode::GetTopics) {
      const bool hasSelection = dialog.ui->topicsList && !dialog.ui->topicsList->selectedItems().isEmpty();
      b->setText("Subscribe");
      b->setEnabled(hasSelection && !_state.req_in_flight);
      return;
    }

    // Other states
    b->setEnabled(false);
  };

  // Refresh button when topic selection changes
  connect(dialog.ui->topicsList, &QTreeWidget::itemSelectionChanged, this, refreshOk);

  // Refresh topic list applying the filter
  connect(dialog.ui->lineEditFilter, &QLineEdit::textChanged,
          this, [&](const QString&) {
            applyTopicFilterKeepSelected(dialog.ui->topicsList, dialog.ui->lineEditFilter->text());
  });

  // =======================
  // OK button logic
  // =======================
  connect(dialog.ui->buttonBox, &QDialogButtonBox::accepted, this, [&]() {

    // Not connected: open socket
    if (!_running) {
      bool ok = false;
      int p = dialog.ui->lineEditPort->text().toUShort(&ok);
      if (!ok) {
        QMessageBox::warning(nullptr, "WebSocket Client", "Invalid Port", QMessageBox::Ok);
        return;
      }
      const QString adrr = dialog.ui->lineEditAddress->text().trimmed();

      if (adrr.isEmpty()) {
        QMessageBox::warning(nullptr, "WebSocket Client", "Invalid Address", QMessageBox::Ok);
        return;
      }

      // BuildWebSocket URL
      _url = QUrl(QString("ws://%1:%2").arg(adrr).arg(p));

      // Disable button while connecting
      auto b = dialog.ui->buttonBox->button(QDialogButtonBox::Ok);
      if (b) b->setEnabled(false);

      // Open WebSocket
      _socket.open(_url);
      return;
    }

    // Already connected: subscribe to topics
    if (_state.mode != WsState::Mode::GetTopics) return;
    if (_state.req_in_flight) return;

    if (!dialog.ui->topicsList) return;
    const auto selected = dialog.ui->topicsList->selectedItems();
    if (selected.isEmpty()) return;

    // Build JSON array with selected topic names
    QJsonArray arr;

    // Refresh selected topics cache
    _topics.clear();
    for (auto* it : selected) {
      const auto name = it->text(0);
      const auto type = it->text(1);

      if (name.isEmpty()) continue;

      arr.append(name);

      // Cache selected topics (schema will be filled after subscribe response)
      TopicInfo info;
      info.name = name;
      info.type = type;
      _topics.push_back(std::move(info));
    }
    if (arr.isEmpty()) return;

    // Update state: one request in-flight
    _state.mode = WsState::Mode::Subscribe;
    _state.req_in_flight = true;

    // Send subscribe command
    QJsonObject cmd;
    cmd["command"] = "subscribe";
    cmd["topics"] = arr;

    // Track expected response
    _pendingMode = WsState::Mode::Subscribe;
    _pendingRequestId = sendCommand(cmd);

    // Disable button until response arrives
    auto b = dialog.ui->buttonBox->button(QDialogButtonBox::Ok);
    if (b) b->setEnabled(false);

    // Close dialog after subscribing (PlotJuggler takes over)
    dialog.reject();
  });

  // =======================
  // Cancel button
  // =======================
  connect(dialog.ui->buttonBox, &QDialogButtonBox::rejected, this, [&]() {
    // Stop everything and close dialog
    shutdown();
    dialog.reject();
  });

  // Run dialog (blocking)
  dialog.exec();
  _dialog = nullptr;

  // Connection failed or cancelled
  if (!_running) {
    shutdown();
    return false;
  }

  // Store selected port
  settings.setValue("WebsocketClient::port", dialog.ui->lineEditPort->text().toInt());
  return true;
}

void WebsocketClient::shutdown()
{
  if (!_running) return;
  _running = false;

  // Stop periodic timers
  _topicsTimer.stop();
  _heartBeatTimer.stop();

  // Reset state machine
  _state.mode = WsState::Mode::Close;
  _state.req_in_flight = false;

  // Reset pending request tracking
  _pendingRequestId.clear();
  _pendingMode = WsState::Mode::Close;

  // Close dialog if still open
  if (_dialog) _dialog->reject();
  _dialog = nullptr;

  // Clean topics cache
  _topics.clear();

#ifdef PJ_BUILD
  // Drop created parsers
  _parsers_topic.clear();

  // Clean data
  dataMap().clear();
  emit dataReceived();
#endif

  // Close socket
  _socket.abort();
  _socket.close();
}

bool WebsocketClient::pause()
{
  // Pause streaming on server side
  if (!_running) return false;
  if (_state.req_in_flight) return false;

  QJsonObject cmd;
  cmd["command"] = "pause";

  return !sendCommand(cmd).isEmpty();
}

bool WebsocketClient::resume()
{
  // Resume streaming on server side
  if (!_running) return false;
  if (_state.req_in_flight) return false;

  QJsonObject cmd;
  cmd["command"] = "resume";

  return !sendCommand(cmd).isEmpty();
}

bool WebsocketClient::unsubscribe()
{
  // Unsubscribe currently selected topics
  if (!_running) return false;
  if (_state.req_in_flight) return false;

  QJsonArray arr;
  for (const auto& t : _topics) {
    if (!t.name.isEmpty())
      arr.append(t.name);
  }

  if (arr.isEmpty()) return false;

  QJsonObject cmd;
  cmd["command"] = "unsubscribe";
  cmd["topics"] = arr;

  return !sendCommand(cmd).isEmpty();
}

void WebsocketClient::onConnected()
{
  _running = true;
  qDebug() << "Connected";

  // First step after connect: request topics
  _state.mode = WsState::Mode::GetTopics;
  _state.req_in_flight = true;

  QJsonObject cmd;
  cmd["command"] = "get_topics";

  // Track expected response
  _pendingMode = WsState::Mode::GetTopics;
  _pendingRequestId = sendCommand(cmd);

  // Start periodic topic refresh
  _topicsTimer.start();
}

void WebsocketClient::onDisconnected()
{
  if (_dialog && _dialog->ui && _dialog->ui->topicsList && _dialog->ui->buttonBox) {
    _dialog->ui->topicsList->clear();
    auto b = _dialog->ui->buttonBox->button(QDialogButtonBox::Ok);
    if (b) b->setEnabled(true);
  }

  if (!_running) return;
  // Stop topic polling
  _topicsTimer.stop();
  _heartBeatTimer.stop();

  // Reset state machine
  _state.mode = WsState::Mode::Close;
  _state.req_in_flight = false;

  // Reset pending request tracking
  _pendingRequestId.clear();
  _pendingMode = WsState::Mode::Close;

  // Clear topics cache
  _topics.clear();

#ifdef PJ_BUILD
  // Drop created parsers
  _parsers_topic.clear();
#endif

  _running = false;
  qDebug() << "Disconnected" << Qt::endl;
}

void WebsocketClient::onError(QAbstractSocket::SocketError)
{
  //Show Qt socket error string
  QMessageBox::warning(nullptr, "WebSocket Client", _socket.errorString(), QMessageBox::Ok);
  onDisconnected();
}

void WebsocketClient::onTextMessageReceived(const QString& message)
{
  if (!_running) return;

  // Parse JSON message
  QJsonParseError err;
  const auto doc = QJsonDocument::fromJson(message.toUtf8(), &err);
  if (err.error != QJsonParseError::NoError || !doc.isObject())
    return;

  const auto obj = doc.object();

  // Validate protocol version
  if (!obj.contains("protocol_version") ||
      obj.value("protocol_version").toInt() != 1)
    return;

  const auto status = obj.value("status").toString();
  const auto id = obj.value("id").toString();

  // If a request is in-flight, only accept matching response "id"
  if (_state.req_in_flight) {
    if (_pendingRequestId.isEmpty() || id != _pendingRequestId)
      return;
  }

  // Error response from server
  if (status == "error") {
    _state.req_in_flight = false;

    // Reset pending request
    _pendingRequestId.clear();
    _pendingMode = WsState::Mode::Close;

    const auto msg = obj.value("message").toString("Unknown error");
    QMessageBox::warning(nullptr, "WebSocket Client", msg, QMessageBox::Ok);
    return;
  }

  // Only handle successful responses
  if (status != "success")
    return;

  // Request completed successfully
  _state.req_in_flight = false;

  // Save mode locally, then clear pending (avoid re-entrancy issues)
  const auto handledMode = _pendingMode;
  _pendingRequestId.clear();
  _pendingMode = WsState::Mode::Close;

  switch (handledMode) {
    case WsState::Mode::GetTopics:
    {
      // Expect array of topics
      if (!obj.contains("topics") || !obj.value("topics").isArray())
        break;

      // Dialog may already be closed
      if (!_dialog || !_dialog->ui || !_dialog->ui->topicsList)
        break;

      auto* view = _dialog->ui->topicsList;

      // Save current scroll position
      auto* vsb = view->verticalScrollBar();
      const int scroll_y = vsb ? vsb->value() : 0;

      // Save current selection to restore it after refresh
      QStringList selected_topics;
      for (auto* it : view->selectedItems())
        selected_topics << it->text(0);

      // Update UI without triggering signals
      view->setUpdatesEnabled(false);
      view->blockSignals(true);

      view->clear();

      // Populate topic list
      const auto topics = obj.value("topics").toArray();
      for (const auto& v : topics) {
        if (!v.isObject()) continue;

        const auto t = v.toObject();
        const auto name = t.value("name").toString();
        const auto type = t.value("type").toString();
        if (name.isEmpty()) continue;

        auto* item = new QTreeWidgetItem(view);
        item->setText(0, name);
        item->setText(1, type);

        // Restore previous selection
        if (selected_topics.contains(name))
          item->setSelected(true);
      }

      // Apply the filter after restore
      if (_dialog->ui->lineEditFilter) {
        applyTopicFilterKeepSelected(view, _dialog->ui->lineEditFilter->text());
      }

      view->blockSignals(false);
      view->setUpdatesEnabled(true);

      // Restore scroll position after layout update
      QTimer::singleShot(0, view, [view, scroll_y]() {
        if (auto* sb = view->verticalScrollBar())
          sb->setValue(scroll_y);
      });

      break;
    }

    case WsState::Mode::Subscribe:
    {
      // The server must return schemas for the accepted topics.
      // Expected format:
      // "schemas": {
      //   "/topic_a": { "name":"pkg/msg/Type", "encoding":"cdr", "definition":"..." },
      //   "/topic_b": { "name":"...", "encoding":"...", "definition":"..." }
      // }
      if (!obj.contains("schemas") || !obj.value("schemas").isObject()) {
        _topics.clear();
#ifdef PJ_BUILD
        _parsers_topic.clear();
#endif
        break;
      }

      const auto schemas = obj.value("schemas").toObject();

      // Keep only topics that the server confirmed
      _topics.erase(
          std::remove_if(_topics.begin(), _topics.end(),
                         [&](const TopicInfo& t){ return !schemas.contains(t.name); }),
          _topics.end());

      // Fill schema fields per topic
      for (auto& t : _topics) {
        const auto s = schemas.value(t.name).toObject();
        t.schema_name = s.value("name").toString(t.type);
        t.schema_encoding = s.value("encoding").toString();
        t.schema_definition = s.value("definition").toString();
      }

      // Create parsers for accepted topics (PJ build only)
      createParsersForTopics();

      // Move to Data mode and start heartbeat
      _state.mode = WsState::Mode::Data;
      _heartBeatTimer.start();

      break;
    }

    case WsState::Mode::Data:
    {
      // Text messages in data mode currently ignored
      break;
    }

    case WsState::Mode::Close:
      break;
    default:
      qWarning() << "Unhandled mode:" << int(handledMode);
      break;
  }
}

// =======================
// Binary helpers
// =======================
template <typename T>
static bool readLE(const uint8_t*& p, const uint8_t* end, T& out)
{
  // Read POD type from buffer as little-endian
  if (p + sizeof(T) > end) return false;
  std::memcpy(&out, p, sizeof(T));
  out = qFromLittleEndian(out);
  p += sizeof(T);
  return true;
}

bool WebsocketClient::parseDecompressedPayload(const QByteArray& decompressed, uint32_t expected_count)
{
  // Payload format: repeated blocks
  // [u16 topic_name_len][bytes topic_name][u64 ts_ns][u32 cdr_len][bytes cdr]
  const uint8_t* q = reinterpret_cast<const uint8_t*>(decompressed.constData());
  const uint8_t* qend = q + decompressed.size();

  uint32_t parsed = 0;

  // Parse until end of payload
  while (q < qend) {
    uint16_t name_len = 0;
    if (!readLE(q, qend, name_len)) return false;
    if (q + name_len > qend) return false;

    QString topic = QString::fromUtf8(reinterpret_cast<const char*>(q), name_len);
    q += name_len;

    uint64_t ts_ns = 0;
    if (!readLE(q, qend, ts_ns)) return false;
    double ts_sec = double(ts_ns) * 1e-9;

    uint32_t data_len = 0;
    if (!readLE(q, qend, data_len)) return false;
    if (q + data_len > qend) return false;

    // CDR buffer points inside decompressed payload
    const uint8_t* cdr = q;
    q += data_len;

    // Push message into parser / PlotJuggler
    onRos2CdrMessage(topic, ts_sec, cdr, data_len);
    parsed++;
  }

  // Header message_count must match parsed messages
  if (parsed != expected_count) {
    qWarning() << "Parsed messages mismatch. header=" << expected_count
               << "parsed=" << parsed
               << "decompressed=" << decompressed.size();
    return false;
  }

  return true;
}

void WebsocketClient::onBinaryMessageReceived(const QByteArray& message)
{
  if (!_running) return;

  // Frame header must be at least 16 bytes
  if (message.size() < 16) {
    return;
  }

  // Frame header fields (little-endian)
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(message.constData());
  const uint8_t* end = ptr + message.size();

  uint32_t magic = 0;
  uint32_t message_count = 0;
  uint32_t uncompressed_size = 0;
  uint32_t flags = 0;

  if (!readLE(ptr, end, magic)) return;
  if (!readLE(ptr, end, message_count)) return;
  if (!readLE(ptr, end, uncompressed_size)) return;
  if (!readLE(ptr, end, flags)) return;

  // Validate magic and flags
  if (magic != 0x42524A50) { // "PJRB"
    qWarning() << "Bad magic:" << Qt::hex << magic;
    return;
  }
  if (flags != 0) {
    qWarning() << "Bad flag:" << flags;
    return;
  }

  // Compressed payload starts after 16-byte header
  QByteArray compressed = message.mid(16);
  if (compressed.isEmpty())
    return;

  // ZSTD decompress
  QByteArray decompressed;
  decompressed.resize(int(uncompressed_size));

  size_t res = ZSTD_decompress(decompressed.data(),
                               size_t(decompressed.size()),
                               compressed.constData(),
                               size_t(compressed.size()));

  if (ZSTD_isError(res)) {
    qWarning() << "ZSTD_decompress error:" << ZSTD_getErrorName(res);
    return;
  }

  // Resize to actual decompressed bytes
  decompressed.resize(int(res));

  // Parse messages inside payload
  parseDecompressedPayload(decompressed, message_count);
}

// =======================
// Commands / requests
// =======================
QString WebsocketClient::sendCommand(QJsonObject obj)
{
  if (_socket.state() != QAbstractSocket::ConnectedState) return QString();

  // Every command must have a "command" field
  if (!obj.contains("command"))
    return QString();

  // Generate unique ID if missing
  if (!obj.contains("id"))
    obj["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);

  // Addprotocol version if missing
  if (!obj.contains("protocol_version"))
    obj["protocol_version"] = 1;

  // Serialize and send JSON
  QJsonDocument doc(obj);
  _socket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

  return obj["id"].toString();
}

void WebsocketClient::requestTopics()
{
  if (_socket.state() != QAbstractSocket::ConnectedState) return;

  // Only poll when connected and idle
  if (!_running) return;
  if (_state.mode != WsState::Mode::GetTopics) return;
  if (_state.req_in_flight) return;

  _state.req_in_flight = true;

  QJsonObject cmd;
  cmd["command"] = "get_topics";

  // Track expected response
  _pendingMode = WsState::Mode::GetTopics;
  _pendingRequestId = sendCommand(cmd);
}

void WebsocketClient::sendHeartBeat()
{
  if (_socket.state() != QAbstractSocket::ConnectedState) return;

  // Heartbeat only in Data mode
  if (!_running) return;
  if (_state.mode != WsState::Mode::Data) return;

  // Keep-alive / watchdog on server side
  QJsonObject cmd;
  cmd["command"] = "heartbeat";
  sendCommand(cmd);
}

// =======================
// PlotJuggler integration
// =======================
void WebsocketClient::createParsersForTopics()
{
#ifdef PJ_BUILD
   // Create one parser per subscribed topic using PlotJuggler factories
  for (const auto& t : _topics) {

    const std::string topic_name = t.name.toStdString();

    // Already created
    if (_parsers_topic.count(topic_name) != 0)
      continue;

    // IMPORTANT: factories are indexed by QString
    const QString encoding_q = t.schema_encoding;
    const QString schema_name_q = t.schema_name;

    // Find parser factory by encoding (QString key)
    auto factories = parserFactories();
    auto it = factories->find(encoding_q);
    if (it == factories->end()) {

      // Warn only once per encoding
      static std::set<QString> warned;
      if (warned.insert(encoding_q).second) {
        QMessageBox::warning(
            nullptr,
            "Encoding problem",
            QString("No parser available for encoding [%0]").arg(encoding_q));
      }
      continue;
    }

    // Convert to std::string only for the createParser call (common in PJ)
    const std::string schema_name = schema_name_q.toStdString();
    const std::string definition  = t.schema_definition.toStdString();

    // Create parser instance
    PJ::MessageParserPtr parser =
        it->second->createParser(topic_name,
                                 schema_name,
                                 definition,
                                 dataMap());

    if (!parser)
      continue;

    _parsers_topic.emplace(topic_name, std::move(parser));
  }
#endif
}

void WebsocketClient::onRos2CdrMessage(const QString& topic, double ts_sec, const uint8_t* cdr, uint32_t len)
{
#ifdef PJ_BUILD
   // Route CDR blob to the parser created for this topic
  const auto key = topic.toStdString();
  auto it = _parsers_topic.find(key);
  if (it == _parsers_topic.end())
    return;

  PJ::MessageRef msg_ref(cdr, len);
  it->second->parseMessage(msg_ref, ts_sec);

  // Notify PlotJuggler that new data is available
  emit dataReceived();
#else
  // Debug build: just log reception
  Q_UNUSED(cdr);
  Q_UNUSED(len);
  qDebug() << "RX msg topic=" << topic << "ts=" << ts_sec << "cdr=" << len << Qt::endl;
#endif
}
