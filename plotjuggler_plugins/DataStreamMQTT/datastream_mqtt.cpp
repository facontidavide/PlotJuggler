#include "datastream_mqtt.h"
#include "ui_datastream_mqtt.h"
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QUuid>
#include <QIntValidator>
#include <QMessageBox>

DataStreamMQTT::DataStreamMQTT():
  _running(false)
{
   _notification_action = new QAction(this);

  connect(_notification_action, &QAction::triggered, this, [this]() {
    QMessageBox::warning(nullptr, "MQTT error",
                         QString("Failed to parse %1 messages").arg(_failed_parsing),
                         QMessageBox::Ok);

    if (_failed_parsing > 0)
    {
      _failed_parsing = 0;
      emit notificationsChanged(_failed_parsing);
    }
  });

  _mosq = std::make_shared<MQTTClient>();
  _dialog = new MQTT_Dialog(_mosq);
}

DataStreamMQTT::~DataStreamMQTT()
{
  shutdown();
  delete _dialog;
}

bool DataStreamMQTT::start(QStringList *)
{
  if (_running)
  {
    return _running;
  }

  //cleanup notifications
  _failed_parsing = 0;
  emit notificationsChanged(0);

  if( !availableParsers() )
  {
    QMessageBox::warning(nullptr,tr("MQTT Client"), tr("No available MessageParsers"),  QMessageBox::Ok);
    _running = false;
    return false;
  }

  bool first_start = _dialog->ui->comboBoxProtocol->count() == 0;

  if( first_start )
  {
    // change the section of the dialog related to protocols
    for( const auto& it: *availableParsers())
    {
      _dialog->ui->comboBoxProtocol->addItem( it.first );

      if(auto widget = it.second->optionsWidget() )
      {
        widget->setVisible(false);
        _dialog->ui->layoutOptions->addWidget( widget );
      }
    }

    connect(_dialog->ui->comboBoxProtocol,
            qOverload<const QString &>(&QComboBox::currentIndexChanged),
            this, &DataStreamMQTT::onComboProtocolChanged);
  }

  _running = false;

  QSettings settings;
  _protocol = settings.value("MosquittoMQTT::serialization_protocol", "JSON").toString();
  _dialog->ui->comboBoxProtocol->setCurrentText(_protocol);

  if( _dialog->exec() == QDialog::Rejected )
  {
    return false;
  }
  _protocol = _dialog->ui->comboBoxProtocol->currentText();

  // remove all previous subscriptions and create new ones
  for( const auto& topic: _mosq->config().topics )
  {
    _mosq->unsubscribe(topic);
  }

  for (const auto& item: _dialog->ui->listWidget->selectedItems())
  {
    MQTTClient::TopicCallback callback = [this](const mosquitto_message* message)
    {
      onMessageReceived(message);
    };

    std::string topic_name = item->text().toStdString();
    _mosq->subscribe(topic_name, _mosq->config().qos);
    _mosq->addMessageCallback(topic_name, callback);
  }

  _running = true;
  return _running;
}

void DataStreamMQTT::shutdown()
{
  if( _running )
  {
    _running = false;
    _parsers.clear();
    _topic_to_parse.clear();
    dataMap().clear();
  }
}

bool DataStreamMQTT::isRunning() const
{
  return _running;
}


void DataStreamMQTT::onComboProtocolChanged(const QString& selected_protocol)
{
  if (_current_parser_creator)
  {
    if( auto prev_widget = _current_parser_creator->optionsWidget())
    {
      prev_widget->setVisible(false);
    }
  }
  _current_parser_creator = availableParsers()->at(selected_protocol);

  if (auto widget = _current_parser_creator->optionsWidget())
  {
    widget->setVisible(true);
  }
}

void DataStreamMQTT::onMessageReceived(const mosquitto_message *message)
{
  std::unique_lock<std::mutex> lk(mutex());

  auto it = _parsers.find(message->topic);
  if( it == _parsers.end() )
  {
    auto& parser_factory = availableParsers()->at( _protocol );
    auto parser = parser_factory->createInstance({}, dataMap());
    it = _parsers.insert( {message->topic, parser} ).first;
  }
  auto& parser = it->second;

  bool result = false;
  try {
    MessageRef msg( static_cast<uint8_t*>(message->payload), message->payloadlen);

    using namespace std::chrono;
    auto ts = high_resolution_clock::now().time_since_epoch();
    double timestamp = 1e-6* double( duration_cast<microseconds>(ts).count() );

    result = parser->parseMessage(msg, timestamp);
  }
  catch (std::exception& ) {}

  emit dataReceived();

  if( !result )
  {
    _failed_parsing++;
    emit notificationsChanged(_failed_parsing);
  }
}

