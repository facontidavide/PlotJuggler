#pragma once

#include <QObject>
#include <QWebSocket>
#include <QUrl>
#include <QByteArray>
#include <QString>
#include <QAbstractSocket>
#include <QTimer>
#include <QJsonObject>

#include <vector>
#include <utility>

struct WsState
{
    enum class Mode
    {
        GetTopics,
        Subscribe,
        Data,
        Close
    };

    Mode mode = Mode::Close;
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

    char* name() const
    {
        return "WebSocket Server";
    }

    bool isDebugPlugin() const
    {
        return false;
    }

    bool pause();
    bool resume();
    bool unsubscribe();

signals:
    void closed();

private:
    QWebSocket _socket;
    QUrl _url;
    bool _running;
    WsState _state;

    WebsocketDialog* _dialog;
    QTimer _topicsTimer;
    QTimer _heartBeatTimer;

    std::vector<std::pair<QString, QString>> _topics;

#ifdef PJ_BUILD
    PJ::CompositeParser _parser;
    PJ::RosParserConfig _config;
#endif

    QString sendCommand(QJsonObject obj);
    QString _pendingRequestId;
    WsState::Mode _pendingMode = WsState::Mode::Close;

    void requestTopics();
    void sendHeartBeat();
    void createParsersForTopics();
    void onRos2CdrMessage(const QString& topic, double ts_sec, const uint8_t* cdr, uint32_t len);
    bool parseDecompressedPayload(const QByteArray& decompressed, uint32_t expected_count);

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& message);
    void onBinaryMessageReceived(const QByteArray& message);
    void onDisconnected();
    void onError(QAbstractSocket::SocketError);
};
