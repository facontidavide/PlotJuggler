#include "websocket_client.h"

#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QMessageBox>
#include <QPushButton>
#include <QIntValidator>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QAbstractButton>

#include "ui_websocket_client.h"

class WebsocketDialog : public QDialog
{
    Q_OBJECT
public:
    WebsocketDialog() : QDialog(nullptr), ui(new Ui::WebSocketDialog)
    {
        ui->setupUi(this);
        setWindowTitle("WebSocket Client");

        ui->lineEditPort->setValidator(new QIntValidator(1, 65535, this));

        connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &WebsocketDialog::connectRequested);
    }

    ~WebsocketDialog() { delete ui; }

signals:
    void connectRequested();

public:
    Ui::WebSocketDialog* ui;
};

WebsocketClient::WebsocketClient() : _running(false), _dialog(nullptr)
{
    _state.mode = WsState::Mode::Close;
    _state.req_in_flight = false;

    _topicsTimer.setInterval(1000);
    connect(&_topicsTimer, &QTimer::timeout, this, &WebsocketClient::requestTopics);

    connect(&_socket, &QWebSocket::connected, this, &WebsocketClient::onConnected);
    connect(&_socket, &QWebSocket::textMessageReceived, this, &WebsocketClient::onTextMessageReceived);
    connect(&_socket, &QWebSocket::binaryMessageReceived, this, &WebsocketClient::onBinaryMessageReceived);
    connect(&_socket, &QWebSocket::disconnected, this, &WebsocketClient::onDisconnected);
    connect(&_socket, &QWebSocket::errorOccurred, this, &WebsocketClient::onError);
}

WebsocketClient::~WebsocketClient()
{
    shutdown();
}

bool WebsocketClient::start()
{
    if (_running) return true;

    QSettings settings;
    int port = settings.value("WebsocketClient::port", 8080).toInt();

    WebsocketDialog dialog;
    _dialog = &dialog;

    dialog.ui->lineEditPort->setText(QString::number(port));

    auto okBtn = dialog.ui->buttonBox->button(QDialogButtonBox::Ok);
    if (okBtn) okBtn->setText("Connect");

    connect(&dialog, &WebsocketDialog::connectRequested, this, [&]() {
        bool ok = false;
        int p = dialog.ui->lineEditPort->text().toUShort(&ok);
        if (!ok)
        {
            QMessageBox::warning(nullptr, "WebSocket Client", "Invalid Port", QMessageBox::Ok);
            return;
        }

        _url = QUrl(QString("ws://127.0.0.1:%1").arg(p));

        auto b = dialog.ui->buttonBox->button(QDialogButtonBox::Ok);
        if (b) b->setEnabled(false);

        _socket.open(_url);
    });

    dialog.exec();
    _dialog = nullptr;

    if (!_running)
    {
        shutdown();
        return false;
    }

    settings.setValue("WebsocketClient::port", dialog.ui->lineEditPort->text().toInt());
    return true;
}

void WebsocketClient::shutdown()
{
    _socket.close();
    _running = false;
}

void WebsocketClient::onConnected()
{
    _running = true;
    qDebug() << "Connected";

    _state.mode = WsState::Mode::GetTopics;
    _state.req_in_flight = true;

    QJsonObject cmd;
    cmd["command"] = "get_topics";
    sendCommand(cmd);

    _topicsTimer.start();
}

void WebsocketClient::onTextMessageReceived(const QString& message)
{
    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(message.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    const auto obj = doc.object();

    if (!obj.contains("protocol_version") ||
        obj.value("protocol_version").toInt() != 1)
        return;

    const auto status = obj.value("status").toString();

    if (status == "error")
    {
        _state.req_in_flight = false;

        const auto msg = obj.value("message").toString("Unknown error");
        QMessageBox::warning(nullptr, "WebSocket Client", msg, QMessageBox::Ok);
        return;
    }

    if (status != "success")
        return;

    switch (_state.mode) {
        case WsState::Mode::GetTopics:
        {
            if (!obj.contains("topics") || !obj.value("topics").isArray())
                break;

            _state.req_in_flight = false;

            if (!_dialog || !_dialog->ui || !_dialog->ui->topicsList)
                break;

            const auto topics = obj.value("topics").toArray();
            _dialog->ui->topicsList->clear();

            for (const auto& v : topics)
            {
                if (!v.isObject()) continue;

                const auto t = v.toObject();
                const auto name = t.value("name").toString();
                const auto type = t.value("type").toString();

                if (name.isEmpty()) continue;

                auto* item = new QTreeWidgetItem();
                item->setText(0, name);
                item->setText(1, type);
                _dialog->ui->topicsList->addTopLevelItem(item);
            }
            break;
        }

        case WsState::Mode::Subscribe:
        {
            _state.req_in_flight = false;
            _state.mode = WsState::Mode::Data;
            break;
        }

        case WsState::Mode::Data:
        {
            break;
        }

        case WsState::Mode::Close:
            break;
        default:
            qWarning() << "Unhandled mode:" << int(_state.mode);
            break;
    }
}

void WebsocketClient::onBinaryMessageReceived(const QByteArray& message)
{
    qDebug() << "RX binary:" << message.size() << "bytes";
}

void WebsocketClient::onDisconnected()
{
    _running = false;
    qDebug() << "Disconnected";
}

void WebsocketClient::onError(QAbstractSocket::SocketError)
{
    _running = false;

    if (_dialog)
    {
        auto b = _dialog->ui->buttonBox->button(QDialogButtonBox::Ok);
        if (b) b->setEnabled(true);
    }

    QMessageBox::warning(nullptr, "WebSocket Client", _socket.errorString(), QMessageBox::Ok);
}

QString WebsocketClient::sendCommand(QJsonObject obj)
{
    if (!obj.contains("command"))
        return QString();

    if (!obj.contains("id"))
        obj["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);

    if (!obj.contains("protocol_version"))
        obj["protocol_version"] = 1;

    QJsonDocument doc(obj);
    _socket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    return obj["id"].toString();
}

void WebsocketClient::requestTopics()
{
    if (!_running) return;
    if (_state.mode != WsState::Mode::GetTopics) return;
    if (_state.req_in_flight) return;

    _state.req_in_flight = true;

    QJsonObject cmd;
    cmd["command"] = "get_topics";
    sendCommand(cmd);
}

#include "websocket_client.moc"
