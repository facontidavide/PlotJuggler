#pragma once

#include <QWebSocket>
#include <QTimer>

#include <unordered_map>

#include "websocket_client_config.h"
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

namespace Ui { class WebSocketDialog; }

class WebsocketDialog;

class WebsocketClient : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  WebsocketClient();

  const std::vector<QAction*>& availableActions() override;

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

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;
  bool xmlLoadState(const QDomElement& parent_element) override;

  bool pause();
  bool resume();
  bool unsubscribe();

private:
  QAction* _action_settings = nullptr;
  std::vector<QAction*> _actions;

  websocket_client_config _config;

  QWebSocket _socket;
  QUrl _url;
  bool _running;
  bool _closing;
  bool _paused;
  WsState _state;

  WebsocketDialog* _dialog;
  QTimer _topicsTimer;
  QTimer _heartBeatTimer;

  std::vector<TopicInfo> _topics;

#ifdef PJ_BUILD
  std::unordered_map<std::string, PJ::MessageParserPtr> _parsers_topic;
#endif

  QString sendCommand(const QJsonObject obj);
  QString _pendingRequestId;
  WsState::Mode _pendingMode = WsState::Mode::Close;

  void saveDefaultSettings();
  void loadDefaultSettings();
  void setupSettings();

  void updateOkButton();

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
