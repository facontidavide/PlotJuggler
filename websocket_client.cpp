#include "websocket_client.h"

#include <QSettings>
#include <QMessageBox>
#include <QIntValidator>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QTimer>

#include "ui_websocket_client.h"

class WebsocketDialog : public QDialog
{
public:
    WebsocketDialog() : QDialog(nullptr), ui(new Ui::WebSocketDialog)
    {
        ui->setupUi(this);
        setWindowTitle("WebSocket Client");

        ui->lineEditPort->setValidator(new QIntValidator(1, 65535, this));

        connect(ui->buttonBox, &QDialogButtonBox::accepted,
                this, &QDialog::accept);
        connect(ui->buttonBox, &QDialogButtonBox::rejected,
                this, &QDialog::reject);
    }

    ~WebsocketDialog()
    {
        delete ui;
    }

    Ui::WebSocketDialog* ui;
};

WebsocketClient::WebsocketClient() : _running(false)
{
    connect(&_socket, &QWebSocket::connected,
            this, &WebsocketClient::onConnected);
    connect(&_socket, &QWebSocket::textMessageReceived,
            this, &WebsocketClient::onTextMessageReceived);
    connect(&_socket, &QWebSocket::binaryMessageReceived,
            this, &WebsocketClient::onBinaryMessageReceived);
    connect(&_socket, &QWebSocket::disconnected,
            this, &WebsocketClient::onDisconnected);
    connect(&_socket, &QWebSocket::errorOccurred,
            this, &WebsocketClient::onError);
}

WebsocketClient::~WebsocketClient()
{
    shutdown();
}

bool WebsocketClient::start()
{
    if (_running) return true;

    QSettings settings;
    QString protocol =
        settings.value("WebsocketClient::protocol", "JSON").toString();
    int port =
        settings.value("WebsocketClient::port", 8080).toInt();

    WebsocketDialog dialog;

    dialog.ui->lineEditPort->setText(QString::number(port));

    dialog.ui->comboBoxProtocol->clear();
    dialog.ui->comboBoxProtocol->addItems({ "JSON" });

    int idx = dialog.ui->comboBoxProtocol->findText(protocol);
    dialog.ui->comboBoxProtocol->setCurrentIndex(idx >= 0 ? idx : 0);

    if (dialog.exec() != QDialog::Accepted)
    {
        _running = false;
        return false;
    }

    bool ok = false;
    port = dialog.ui->lineEditPort->text().toUShort(&ok);
    protocol = dialog.ui->comboBoxProtocol->currentText();

    if (!ok)
    {
        QMessageBox::warning(nullptr,
                             "WebSocket Client",
                             "Puerto inválido",
                             QMessageBox::Ok);
        _running = false;
        return false;
    }

    settings.setValue("WebsocketClient::protocol", protocol);
    settings.setValue("WebsocketClient::port", port);

    _url = QUrl(QString("ws://127.0.0.1:%1").arg(port));

    qDebug() << "Connecting to" << _url.toString()
             << "protocol:" << protocol;

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool connected = false;

    auto c1 = connect(&_socket, &QWebSocket::connected,
                      this, [&]() {
                          connected = true;
                          loop.quit();
                      });

    auto c2 = connect(&_socket, &QWebSocket::errorOccurred,
                      this, [&](QAbstractSocket::SocketError) {
                          connected = false;
                          loop.quit();
                      });

    connect(&timer, &QTimer::timeout,
            this, [&]() {
                connected = false;
                loop.quit();
            });

    _socket.open(_url);
    timer.start(2000);
    loop.exec();

    disconnect(c1);
    disconnect(c2);

    if (!connected)
    {
        QMessageBox::warning(nullptr,
                             "WebSocket Client",
                             "No se pudo conectar",
                             QMessageBox::Ok);
        _socket.close();
        _running = false;
        return false;
    }

    _running = true;
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
}

void WebsocketClient::onTextMessageReceived(const QString& message)
{
    qDebug() << "RX:" << message;
}

void WebsocketClient::onBinaryMessageReceived(const QByteArray& message)
{
    qDebug() << "RX:" << message.size() << "bytes";
}

void WebsocketClient::onDisconnected()
{
    _running = false;
    qDebug() << "Disconnected";
    emit closed();
}

void WebsocketClient::onError(QAbstractSocket::SocketError)
{
    _running = false;
    QMessageBox::warning(nullptr,
                         "WebSocket Client",
                         _socket.errorString(),
                         QMessageBox::Ok);
    emit closed();
}
