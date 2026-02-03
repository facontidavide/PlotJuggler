#pragma once

#include <QObject>
#include <QWebSocket>
#include <QUrl>
#include <QByteArray>
#include <QString>
#include <QAbstractSocket>

namespace Ui { class WebSocketDialog; }

class WebsocketDialog;

class WebsocketClient : public QObject
{
    Q_OBJECT

public:
    WebsocketClient();

    bool start();

    void shutdown();

    bool isRunning()
    {
        return _running;
    }

    ~WebsocketClient();

    char* name()
    {
        return "WebSocket Server";
    }

    bool isDebugPlugin()
    {
        return false;
    }

signals:
    void closed();

private:
    QWebSocket _socket;
    QUrl _url;
    bool _running;

    WebsocketDialog* _dialog;

    QString sendCommand(QJsonObject obj);

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& message);
    void onBinaryMessageReceived(const QByteArray& message);
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);
};
