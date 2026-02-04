#include "websocket_client.h"

#include <QSettings>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

#include <QMessageBox>
#include <QPushButton>
#include <QIntValidator>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTreeWidget>
#include <QtEndian>
#include <cstring>

#include <zstd.h>

#include "ui_websocket_client.h"

// =======================
// Connection dialog
// =======================
class WebsocketDialog : public QDialog
{
    Q_OBJECT
public:
    WebsocketDialog() : QDialog(nullptr), ui(new Ui::WebSocketDialog)
    {
        // Build UI
        ui->setupUi(this);

        // Window title
        setWindowTitle("WebSocket Client");

        // Allow only valid TCP ports
        ui->lineEditPort->setValidator(new QIntValidator(1, 65535, this));

        // Cancel button
        connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &WebsocketDialog::cancelRequested);
        // OK button (connect / subscribe)
        connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &WebsocketDialog::connectRequested);
    }

    ~WebsocketDialog() { delete ui; }

signals:
    // Emitted when OK is pressed
    void connectRequested();

    // Emitted when Cancel is pressed
    void cancelRequested();

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

    // Timer used to periodically request topics
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

WebsocketClient::~WebsocketClient()
{
    shutdown();
}

// =======================
// Start client
// =======================
bool WebsocketClient::start()
{
    // Already running
    if (_running) return true;

    // Load stored port (default 8080)
    QSettings settings;
    int port = settings.value("WebsocketClient::port", 8080).toInt();

    // Create dialog
    WebsocketDialog dialog;
    _dialog = &dialog;

    dialog.ui->lineEditPort->setText(QString::number(port));

    // Rename OK button
    auto okBtn = dialog.ui->buttonBox->button(QDialogButtonBox::Ok);
    if (okBtn) okBtn->setText("Connect");

    // Enable/disable OK button depending on state
    auto refreshOk = [&]() {
        if (!dialog.ui->buttonBox) return;
        auto b = dialog.ui->buttonBox->button(QDialogButtonBox::Ok);
        if (!b) return;

        // Not connected yet
        if (!_running) {
            b->setEnabled(true);
            b->setText("Connect");
            return;
        }

        // Connected and waiting for topic selection
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

    // =======================
    // OK button logic
    // =======================
    connect(&dialog, &WebsocketDialog::connectRequested, this, [&]() {

        // Not connected: open socket
        if (!_running) {
            bool ok = false;
            int p = dialog.ui->lineEditPort->text().toUShort(&ok);
            if (!ok) {
                QMessageBox::warning(nullptr, "WebSocket Client", "Invalid Port", QMessageBox::Ok);
                return;
            }

            // Build WebSocket URL
            _url = QUrl(QString("ws://127.0.0.1:%1").arg(p));

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

        _topics.clear();
        for (auto* it : selected) {
            const auto name = it->text(0);
            const auto type = it->text(1);

            if (name.isEmpty()) continue;

            arr.append(name);

            // Store selected topics
            if (!type.isEmpty()) _topics.push_back({name, type});

        }
        if (arr.isEmpty()) return;

        // Update state
        _state.mode = WsState::Mode::Subscribe;
        _state.req_in_flight = true;

        // Send subscribe command
        QJsonObject cmd;
        cmd["command"] = "subscribe";
        cmd["topics"] = arr;

        // qDebug() << cmd << Qt::endl;
        sendCommand(cmd);

        // Disable button until response
        auto b = dialog.ui->buttonBox->button(QDialogButtonBox::Ok);
        if (b) b->setEnabled(false);
        dialog.reject();
    });

    // =======================
    // Cancel button
    // =======================
    connect(&dialog, &WebsocketDialog::cancelRequested, this, [&]() {
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
    if (!_running && _state.mode == WsState::Mode::Close)
        return;

    _running = false;

    // Stop periodic timers
    _topicsTimer.stop();
    _heartBeatTimer.stop();

    // Reset state
    _state.mode = WsState::Mode::Close;
    _state.req_in_flight = false;

    _dialog = nullptr;

    // Clean topics
    _topics.clear();

    // Close socket
    _socket.disconnect(this);
    _socket.abort();
    _socket.close();
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
    sendCommand(cmd);

    // Start periodic topic refresh
    _topicsTimer.start();
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

    // Error response from server
    if (status == "error")
    {
        _state.req_in_flight = false;

        const auto msg = obj.value("message").toString("Unknown error");
        QMessageBox::warning(nullptr, "WebSocket Client", msg, QMessageBox::Ok);
        return;
    }

    // Only handle successful responses
    if (status != "success") // INTEGRATE PARTIAL
        return;

    switch (_state.mode) {
        case WsState::Mode::GetTopics:
        {
            // Expect array of topics
            if (!obj.contains("topics") || !obj.value("topics").isArray())
                break;

            _state.req_in_flight = false;

            // Dialog may already be closed
            if (!_dialog || !_dialog->ui || !_dialog->ui->topicsList)
                break;

            const auto topics = obj.value("topics").toArray();

            // Save current selection
            QStringList selected_topics;
            for (auto* it : _dialog->ui->topicsList->selectedItems())
                selected_topics << it->text(0);

            // Update UI without triggering signals
            _dialog->ui->topicsList->setUpdatesEnabled(false);
            _dialog->ui->topicsList->blockSignals(true);

            _dialog->ui->topicsList->clear();

            // Populate topic list
            for (const auto& v : topics)
            {
                if (!v.isObject()) continue;

                const auto t = v.toObject();
                const auto name = t.value("name").toString();
                const auto type = t.value("type").toString();
                if (name.isEmpty()) continue;

                auto* item = new QTreeWidgetItem(_dialog->ui->topicsList);
                item->setText(0, name);
                item->setText(1, type);

                // Restore previous selection
                if (selected_topics.contains(name))
                    item->setSelected(true);
            }

            _dialog->ui->topicsList->blockSignals(false);
            _dialog->ui->topicsList->setUpdatesEnabled(true);

            break;
        }

        case WsState::Mode::Subscribe:
        {
            _state.req_in_flight = false;

            if (obj.contains("schemas") && obj.value("schemas").isObject()) {
                const auto schemas = obj.value("schemas").toObject();

                _topics.erase(
                    std::remove_if(_topics.begin(), _topics.end(),
                            [&](const auto& p){ return !schemas.contains(p.first); }),
                              _topics.end());
            }
            else {
                _topics.clear();
            }

            createParsersForTopics();

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
            qWarning() << "Unhandled mode:" << int(_state.mode);
            break;
    }
}

template <typename T>
static bool readLE(const uint8_t*& p, const uint8_t* end, T& out)
{
    if (p + sizeof(T) > end) return false;
    std::memcpy(&out, p, sizeof(T));
    out = qFromLittleEndian(out);
    p += sizeof(T);
    return true;
}

bool WebsocketClient::parseDecompressedPayload(const QByteArray& decompressed, uint32_t expected_count)
{
    const uint8_t* q = reinterpret_cast<const uint8_t*>(decompressed.constData());
    const uint8_t* qend = q + decompressed.size();

    uint32_t parsed = 0;

    while (q < qend)
    {
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

        const uint8_t* cdr = q;
        q += data_len;

        onRos2CdrMessage(topic, ts_sec, cdr, data_len);
        parsed++;
    }

    if (parsed != expected_count)
    {
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

    if (message.size() < 16) {
        return;
    }

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

    if (magic != 0x42524A50) {
        qWarning() << "Bad magic:" << Qt::hex << magic;
        return;
    }
    if (flags != 0) {
        qWarning() << "Bad flag:" << flags;
        return;
    }
    QByteArray compressed = message.mid(16);
    if (compressed.isEmpty())
        return;

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

    decompressed.resize(int(res));

    parseDecompressedPayload(decompressed, message_count);
}

void WebsocketClient::onDisconnected()
{
    // Stop topic polling
    _topicsTimer.stop();

    // Reset state
    _state.mode = WsState::Mode::Close;
    _state.req_in_flight = false;

    _running = false;
    qDebug() << "Disconnected" << Qt::endl;
}

void WebsocketClient::onError(QAbstractSocket::SocketError)
{
    _running = false;

    // Re-enable OK button if dialog is still open
    if (_dialog && _dialog->ui && _dialog->ui->buttonBox) {
        auto b = _dialog->ui->buttonBox->button(QDialogButtonBox::Ok);
        if (b) b->setEnabled(true);
    }
    QMessageBox::warning(nullptr, "WebSocket Client", _socket.errorString(), QMessageBox::Ok);
}

QString WebsocketClient::sendCommand(QJsonObject obj)
{
    // Every command must have a "command" field
    if (!obj.contains("command"))
        return QString();

    // Generate unique ID if missing
    if (!obj.contains("id"))
        obj["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Add protocol version if missing
    if (!obj.contains("protocol_version"))
        obj["protocol_version"] = 1;

    // Serialize and send JSON
    QJsonDocument doc(obj);
    _socket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    return obj["id"].toString();
}

void WebsocketClient::requestTopics()
{
    // Only poll when connected and idle
    if (!_running) return;
    if (_state.mode != WsState::Mode::GetTopics) return;
    if (_state.req_in_flight) return;

    _state.req_in_flight = true;

    QJsonObject cmd;
    cmd["command"] = "get_topics";
    sendCommand(cmd);
}

void WebsocketClient::sendHeartBeat()
{
    // Heartbeat only in Data mode
    if (!_running) return;
    if (_state.mode != WsState::Mode::Data) return;

    QJsonObject cmd;
    cmd["command"] = "heartbeat";
    sendCommand(cmd);
}

void WebsocketClient::createParsersForTopics()
{
#ifdef PJ_BUILD
    for (const auto& [topic_, type_] : _topics)
    {
        const auto topic = topic_.toStdString();
        const auto type  = type_.toStdString();

        if (!_parser.hasParser(topic))
        {
            _parser.addParser(topic,
                              CreateParserROS2(*parserFactories(), topic, type, dataMap()));
        }
    }
#endif
}

void WebsocketClient::onRos2CdrMessage(const QString& topic, double ts_sec, const uint8_t* cdr, uint32_t len)
{
#ifdef PJ_BUILD
        const std::string topic_std = topic.toStdString();

        if (!_parser.hasParser(topic_std))
            return;

        PJ::MessageRef msg_ref(cdr, len);
        _parser.parseMessage(topic_std, msg_ref, ts_sec);

        emit dataReceived();
#else
        Q_UNUSED(cdr);
        Q_UNUSED(len);
        qDebug() << "RX msg topic=" << topic << "ts=" << ts_sec << "cdr=" << len << Qt::endl;
#endif
}

// Qt MOC include for QObject/Q_OBJECT
#include "websocket_client.moc"
