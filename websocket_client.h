#pragma once

#include <QObject>
#include <QWebSocket>
#include <QUrl>
#include <QByteArray>
#include <QString>
#include <QAbstractSocket>
#include <QTimer>

struct WsState
{
    enum class Mode
    {
        GetTopics,
        Subscribe,
        Data,
        Close
    };

    Mode mode = Mode::GetTopics;
    bool req_in_flight = false;
};

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
    WsState _state;

    WebsocketDialog* _dialog;
    QTimer _topicsTimer;

    QString sendCommand(QJsonObject obj);
    void requestTopics();

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& message);
    void onBinaryMessageReceived(const QByteArray& message);
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);
};
