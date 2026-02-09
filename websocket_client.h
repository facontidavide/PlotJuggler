#pragma once

#include <QObject>
#include <QWebSocket>
#include <QUrl>
#include <QByteArray>
#include <QString>
#include <QAbstractSocket>
#include <QTimer>
#include <QJsonObject>
#include <QtPlugin>

#include <vector>
#include <utility>

#include "PlotJuggler/datastreamer_base.h"
#include "PlotJuggler/messageparser_base.h"

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

struct TopicInfo
{
  QString name;
  QString type;
  QString schema_name;
  QString schema_encoding;
  QString schema_definition;
};

namespace Ui { class WebSocketSettings; class WebsocketDialog; }

class WebSocketSettings;
class WebsocketDialog;

class WebsocketClient : public PJ::DataStreamer
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
    Q_INTERFACES(PJ::DataStreamer)

public:
    WebsocketClient();

    virtual bool start(QStringList*) override;

    virtual void shutdown() override;

    virtual bool isRunning() const override
    {
        return _running;
    }

    ~WebsocketClient() override = default;

    virtual const char* name() const override
    {
        return "WebSocket Client";
    }

    virtual bool isDebugPlugin() override
    {
        return false;
    }

    bool pause();
    bool resume();
    bool unsubscribe();

private:
    QWebSocket _socket;
    QUrl _url;
    bool _running;
    WsState _state;
    double _max_rate;
    bool _keep;

    WebsocketDialog* _dialog;
    WebSocketSettings* _settings;

    QTimer _topicsTimer;
    QTimer _heartBeatTimer;

    std::vector<TopicInfo> _topics;

#ifdef PJ_BUILD
    std::unordered_map<std::string, PJ::MessageParserPtr> _parsers_topic;
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
