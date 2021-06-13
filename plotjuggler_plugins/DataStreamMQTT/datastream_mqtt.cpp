#include "datastream_mqtt.h"
#include "ui_datastream_mqtt.h"
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QUuid>

class MQTT_Dialog: public QDialog
{
public:
  MQTT_Dialog():
    QDialog(nullptr),
    ui(new Ui::DataStreamMQTT)
  {
    ui->setupUi(this);

    static QString uuid =  QString::number(rand());
    ui->lineEditClientID->setText(tr("Plotjuggler-") + uuid);
  }

  ~MQTT_Dialog() {
    while( ui->layoutOptions->count() > 0)
    {
      auto item = ui->layoutOptions->takeAt(0);
      item->widget()->setParent(nullptr);
    }
    delete ui;
  }

  Ui::DataStreamMQTT* ui;
};

//---------------------------------------------

QString Code(int rc)
{
  switch(rc)
  {
  case MQTTASYNC_FAILURE: return"FAILURE";
  case MQTTASYNC_PERSISTENCE_ERROR: return"PERSISTENCE_ERROR";
  case MQTTASYNC_DISCONNECTED: return"DISCONNECTED";
  case MQTTASYNC_MAX_MESSAGES_INFLIGHT: return"MAX_MESSAGES_INFLIGHT";
  case MQTTASYNC_BAD_UTF8_STRING: return"BAD_UTF8_STRING";
  case MQTTASYNC_NULL_PARAMETER: return"NULL_PARAMETER";
  case MQTTASYNC_TOPICNAME_TRUNCATED: return"TOPICNAME_TRUNCATED";
  case MQTTASYNC_BAD_STRUCTURE: return"BAD_STRUCTURE";
  case MQTTASYNC_BAD_QOS: return"BAD_QOS";
  case MQTTASYNC_NO_MORE_MSGIDS: return"NO_MORE_MSGIDS";
  case MQTTASYNC_OPERATION_INCOMPLETE: return"OPERATION_INCOMPLETE";
  case MQTTASYNC_MAX_BUFFERED_MESSAGES: return"MAX_BUFFERED_MESSAGES";
  case MQTTASYNC_SSL_NOT_SUPPORTED: return"SSL_NOT_SUPPORTED";
  case MQTTASYNC_BAD_PROTOCOL: return"BAD_PROTOCOL";
  case MQTTASYNC_BAD_MQTT_OPTION: return"BAD_MQTT_OPTION";
  case MQTTASYNC_WRONG_MQTT_VERSION: return"WRONG_MQTT_VERSION";
  case MQTTASYNC_0_LEN_WILL_TOPIC: return"0_LEN_WILL_TOPIC";
  }
  return QString::number(rc);
}

void ConnectionLost(void *context, char *cause)
{
  DataStreamMQTT* _this = static_cast<DataStreamMQTT*>(context);

  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

  qDebug() <<"MQTT Connection lost. Reconnecting...";

  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;

  int rc = MQTTAsync_connect(_this->_client, &conn_opts);

  if (rc != MQTTASYNC_SUCCESS)
  {
    _this->_error_msg = QString("Failed to start connect, return code %1").arg(Code(rc));
    _this->_finished = true;
  }
}

int MessageArrived(void *context, char *topicName,
                   int topicLen, MQTTAsync_message *message)
{
  DataStreamMQTT* _this = static_cast<DataStreamMQTT*>(context);

  auto it = _this->_parsers.find(topicName);
  if( it == _this->_parsers.end() )
  {
    auto parser = _this->availableParsers()->at( _this->_protocol )->createInstance({}, _this->dataMap());
    it = _this->_parsers.insert( {topicName, parser} ).first;
  }
  auto& parser = it->second;

  try {
    MessageRef msg( static_cast<uint8_t*>(message->payload), message->payloadlen);

    using namespace std::chrono;
    auto ts = high_resolution_clock::now().time_since_epoch();
    double timestamp = 1e-6* double( duration_cast<microseconds>(ts).count() );

    parser->parseMessage(msg, timestamp);

  } catch (std::exception& ) {
    _this->_protocol_issue = true;
    return 0;
  }

//  printf("     topic: %s\n", topicName);
//  printf("   message: %.*s\n", message->payloadlen, (char*)message->payload);
  MQTTAsync_freeMessage(&message);
  MQTTAsync_free(topicName);

  emit _this->dataReceived();
  return 1;
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
  DataStreamMQTT* _this = static_cast<DataStreamMQTT*>(context);
  _this->_disconnection_done = true;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
  DataStreamMQTT* _this = static_cast<DataStreamMQTT*>(context);
  _this->_disconnection_done = true;
}

void onSubscribe(void* context, MQTTAsync_successData* response)
{
  DataStreamMQTT* _this = static_cast<DataStreamMQTT*>(context);
  _this->_subscribed = true;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
  DataStreamMQTT* _this = static_cast<DataStreamMQTT*>(context);
  _this->_error_msg = QString("Subscription Failure. Code %1").arg(Code(response->code));
  _this->_finished = true;
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
  DataStreamMQTT* _this = static_cast<DataStreamMQTT*>(context);
  _this->_error_msg = QString("Connection Failure. Code %1").arg(Code(response->code));
  _this->_finished = true;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
  DataStreamMQTT* _this = static_cast<DataStreamMQTT*>(context);
  MQTTAsync client = _this->_client;

  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

  opts.onSuccess = onSubscribe;
  opts.onFailure = onSubscribeFailure;
  opts.context = _this;

  int rc = MQTTAsync_subscribe(
        client,
        _this->_topic_filter.toStdString().c_str(),
        _this->_qos,
        &opts);

  if ( rc != MQTTASYNC_SUCCESS)
  {
    _this->_error_msg = QString("Failed to start subscribe, return code %1").arg(Code(rc));
    _this->_finished = true;
  }
}


DataStreamMQTT::DataStreamMQTT():
  _protocol_issue(false),
  _running(false)
{
  _protocol_issue_timer.setSingleShot(false);
  _protocol_issue_timer.setInterval(500);

  connect(&_protocol_issue_timer, &QTimer::timeout, this,
          [this](){
    if(_protocol_issue){
      _protocol_issue = false;
      shutdown();

      QMessageBox::warning(nullptr,tr("DataStream MQTT"),
                           tr("Exception while parsing the message. Probably the format was not recognized (%1 used)").arg( this->_protocol),
                           QMessageBox::Ok);
      emit this->closed();

    }
  });
}

bool DataStreamMQTT::start(QStringList *)
{
  if (_running)
  {
    return _running;
  }

  if( !availableParsers() )
  {
    QMessageBox::warning(nullptr,tr("Websocket Server"), tr("No available MessageParsers"),  QMessageBox::Ok);
    _running = false;
    return false;
  }

  MQTT_Dialog* dialog = new MQTT_Dialog();

  for( const auto& it: *availableParsers())
  {
    dialog->ui->comboBoxProtocol->addItem( it.first );

    if(auto widget = it.second->optionsWidget() )
    {
      widget->setVisible(false);
      dialog->ui->layoutOptions->addWidget( widget );
    }
  }

  std::shared_ptr<MessageParserCreator> parser_creator;

  connect(dialog->ui->comboBoxProtocol, qOverload<int>( &QComboBox::currentIndexChanged), this,
          [&](int index)
  {
    if( parser_creator ){
      QWidget*  prev_widget = parser_creator->optionsWidget();
      prev_widget->setVisible(false);
    }
    QString protocol = dialog->ui->comboBoxProtocol->itemText(index);
    parser_creator = availableParsers()->at( protocol );

    if(auto widget = parser_creator->optionsWidget() ){
      widget->setVisible(true);
    }
  });

  // load previous values
  QSettings settings;
  QString address = settings.value("DataStreamMQTT::address").toString();
  _protocol = settings.value("DataStreamMQTT::protocol", "JSON").toString();
  _topic_filter = settings.value("DataStreamMQTT::filter").toString();
  _qos = settings.value("DataStreamMQTT::qos", 0).toInt();
  QString username = settings.value("DataStreamMQTT::username", "").toString();
  QString password = settings.value("DataStreamMQTT::password", "").toString();

  dialog->ui->lineEditAddress->setText( address );
  dialog->ui->comboBoxProtocol->setCurrentText(_protocol);
  dialog->ui->lineEditTopicFilter->setText( _topic_filter );
  dialog->ui->comboBoxQoS->setCurrentIndex(_qos);
  dialog->ui->lineEditUsername->setText(username);
  dialog->ui->lineEditPassword->setText(password);

  if( dialog->exec() == QDialog::Rejected )
  {
    return false;
  }

  address = dialog->ui->lineEditAddress->text();
  _protocol = dialog->ui->comboBoxProtocol->currentText();
  _topic_filter = dialog->ui->lineEditTopicFilter->text();
  _qos = dialog->ui->comboBoxQoS->currentIndex();
  QString cliend_id = dialog->ui->lineEditClientID->text();
  username = dialog->ui->lineEditUsername->text();
  password = dialog->ui->lineEditPassword->text();

  dialog->deleteLater();

  // save back to service
  settings.setValue("DataStreamMQTT::address", address);
  settings.setValue("DataStreamMQTT::filter", _topic_filter);
  settings.setValue("DataStreamMQTT::protocol", _protocol);
  settings.setValue("DataStreamMQTT::qos", _qos);
  settings.setValue("DataStreamMQTT::username", username);
  settings.setValue("DataStreamMQTT::password", password);

  _subscribed = false;
  _finished = false;
  _protocol_issue = false;

  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;

  int rc = MQTTAsync_create(&_client,
                            address.toStdString().c_str(),
                            cliend_id.toStdString().c_str(),
                            MQTTCLIENT_PERSISTENCE_NONE,
                            nullptr);

  if (rc != MQTTASYNC_SUCCESS)
  {
    QMessageBox::warning(nullptr,tr("DataStream MQTT"),
                         tr("Failed create client MQTT. Error code %1").arg(Code(rc)),
                         QMessageBox::Ok);
    return false;
  }

  rc = MQTTAsync_setCallbacks(_client, this, ConnectionLost, MessageArrived, nullptr);
  if (rc != MQTTASYNC_SUCCESS)
  {
    QMessageBox::warning(nullptr,tr("DataStream MQTT"),
                         tr("Failed to set callbacks. Error code %1").arg(Code(rc)),
                         QMessageBox::Ok);
    MQTTAsync_destroy(&_client);
    return false;
  }

  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.onSuccess = onConnect;
  conn_opts.onFailure = onConnectFailure;
  conn_opts.context = this;
  conn_opts.username = username.toStdString().c_str();
  conn_opts.password = password.toStdString().c_str();

  rc = MQTTAsync_connect(_client, &conn_opts);

  if (rc != MQTTASYNC_SUCCESS)
  {
    QMessageBox::warning(nullptr,tr("DataStream MQTT"),
                         tr("Failed to start connection. Error code %1").arg(Code(rc)),
                         QMessageBox::Ok);
    MQTTAsync_destroy(&_client);
    return false;
  }

  while (!_subscribed && !_finished)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  if (_finished)
  {
    QMessageBox::warning(nullptr,tr("DataStream MQTT"),
                         tr("Failed to start connection. Message: %1").arg(_error_msg),
                         QMessageBox::Ok);
    MQTTAsync_destroy(&_client);
    return false;
  }

  _running = true;
  _protocol_issue_timer.start(500);
  return _running;
}

void DataStreamMQTT::shutdown()
{
  if( _running )
  {
    _disconnection_done = false;
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;

    disc_opts.context = this;
    disc_opts.onSuccess = onDisconnect;
    disc_opts.onFailure = onDisconnectFailure;

    int rc = MQTTAsync_disconnect(_client, &disc_opts);
    if (rc != MQTTASYNC_SUCCESS)
    {
      QMessageBox::warning(nullptr,tr("DataStream MQTT"),
                           tr("Failed to disconned cleanly. Error code %1").arg(rc),
                           QMessageBox::Ok);
      MQTTAsync_destroy(&_client);
      _running = false;
      return;
    }

    while (!_disconnection_done)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    MQTTAsync_destroy(&_client);
    _running = false;
    _parsers.clear();
  }
}

bool DataStreamMQTT::isRunning() const
{
  return _running;
}

DataStreamMQTT::~DataStreamMQTT()
{
  shutdown();
}


