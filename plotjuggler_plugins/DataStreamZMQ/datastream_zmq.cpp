#include "datastream_zmq.h"
#include "ui_datastream_zmq.h"

#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QDialog>
#include <QIntValidator>
#include <chrono>
#include "PlotJuggler/messageparser_base.h"

using namespace PJ;

StreamZMQDialog::StreamZMQDialog(QWidget* parent)
  : QDialog(parent), ui(new Ui::DataStreamZMQ)
{
  ui->setupUi(this);
  ui->lineEditPort->setValidator(new QIntValidator());
}

StreamZMQDialog::~StreamZMQDialog()
{
  while (ui->layoutOptions->count() > 0)
  {
    auto item = ui->layoutOptions->takeAt(0);
    item->widget()->setParent(nullptr);
  }
  delete ui;
}

DataStreamZMQ::DataStreamZMQ()
  : _running(false), _zmq_context(), _zmq_socket(_zmq_context, zmq::socket_type::sub)
{
}

DataStreamZMQ::~DataStreamZMQ()
{
  shutdown();
}

bool DataStreamZMQ::start(QStringList*)
{
  if (_running)
  {
    return _running;
  }

  if (parserFactories() == nullptr || parserFactories()->empty())
  {
    QMessageBox::warning(nullptr, tr("UDP Server"), tr("No available MessageParsers"),
                         QMessageBox::Ok);
    _running = false;
    return false;
  }

  bool ok = false;

  StreamZMQDialog* dialog = new StreamZMQDialog();

  for (const auto& it : *parserFactories())
  {
    dialog->ui->comboBoxProtocol->addItem(it.first);

    if (auto widget = it.second->optionsWidget())
    {
      widget->setVisible(false);
      dialog->ui->layoutOptions->addWidget(widget);
    }
  }

  // load previous values
  QSettings settings;
  QString address = settings.value("ZMQ_Subscriber::address", "localhost").toString();
  QString protocol = settings.value("ZMQ_Subscriber::protocol", "JSON").toString();
  QString topics = settings.value("ZMQ_Subscriber::topics", "").toString();

  int port = settings.value("ZMQ_Subscriber::port", 9872).toInt();

  dialog->ui->lineEditAddress->setText(address);
  dialog->ui->lineEditPort->setText(QString::number(port));
  dialog->ui->lineEditTopics->setText(topics);

  ParserFactoryPlugin::Ptr parser_creator;

  connect(dialog->ui->comboBoxProtocol,
          qOverload<const QString&>(&QComboBox::currentIndexChanged), this,
          [&](const QString& selected_protocol) {
            if (parser_creator)
            {
              if (auto prev_widget = parser_creator->optionsWidget())
              {
                prev_widget->setVisible(false);
              }
            }
            parser_creator = parserFactories()->at(selected_protocol);

            if (auto widget = parser_creator->optionsWidget())
            {
              widget->setVisible(true);
            }
          });

  dialog->ui->comboBoxProtocol->setCurrentText(protocol);

  int res = dialog->exec();
  if (res == QDialog::Rejected)
  {
    _running = false;
    return false;
  }

  address = dialog->ui->lineEditAddress->text();
  port = dialog->ui->lineEditPort->text().toUShort(&ok);
  protocol = dialog->ui->comboBoxProtocol->currentText();
  topics = dialog->ui->lineEditTopics->text();

  _parser = parser_creator->createParser({}, {}, {}, dataMap());

  // save back to service
  settings.setValue("ZMQ_Subscriber::address", address);
  settings.setValue("ZMQ_Subscriber::protocol", protocol);
  settings.setValue("ZMQ_Subscriber::port", port);
  settings.setValue("ZMQ_Subscriber::topics", topics);

  _socket_address =
      (dialog->ui->comboBox->currentText() + address + ":" + QString::number(port))
          .toStdString();

  _zmq_socket.connect(_socket_address.c_str());

  parseTopicFilters(topics);
  subscribeTopics();

  _zmq_socket.set(zmq::sockopt::rcvtimeo, 100);

  qDebug() << "ZMQ listening on address" << QString::fromStdString(_socket_address);
  _running = true;

  _receive_thread = std::thread(&DataStreamZMQ::receiveLoop, this);

  dialog->deleteLater();
  return _running;
}

void DataStreamZMQ::shutdown()
{
  if (_running)
  {
    _running = false;

    if (_receive_thread.joinable())
    {
      _receive_thread.join();
    }

    unsubscribeTopics();

    _zmq_socket.disconnect(_socket_address.c_str());
  }
}

void DataStreamZMQ::receiveLoop()
{
  while (_running)
  {
    zmq::message_t recv_msg;
    zmq::recv_result_t result = _zmq_socket.recv(recv_msg);

    if (recv_msg.size() > 0)
    {
      using namespace std::chrono;
      auto ts = high_resolution_clock::now().time_since_epoch();
      double timestamp = 1e-6 * double(duration_cast<microseconds>(ts).count());

      PJ::MessageRef msg(reinterpret_cast<uint8_t*>(recv_msg.data()), recv_msg.size());
      if (parseMessage(msg, timestamp))
      {
        emit this->dataReceived();
      }
    }
  }
}

bool DataStreamZMQ::parseMessage(const PJ::MessageRef& msg, double& timestamp)
{
  try
  {
    std::lock_guard<std::mutex> lock(mutex());
    _parser->parseMessage(msg, timestamp);
    return true;
  }
  catch (...)
  {
    return false;
  }
}

void DataStreamZMQ::parseTopicFilters(const QString& topic_filters)
{
  const QRegExp regex("(,{0,1}\\s+)|(;\\s*)");

  if (topic_filters.trimmed().size() != 0)
  {
    const auto splitted = topic_filters.split(regex);

    for (const auto& topic : splitted)
    {
      _topic_filters.push_back(topic.toStdString());
    }
  }
  else
  {
    _topic_filters.push_back("");
  }
}

void DataStreamZMQ::subscribeTopics()
{
  for (const auto& topic : _topic_filters)
  {
    qDebug() << "ZMQ Subscribed topic" << QString::fromStdString(topic);

    _zmq_socket.set(zmq::sockopt::subscribe, topic);
  }
}

void DataStreamZMQ::unsubscribeTopics()
{
  for (const auto& topic : _topic_filters)
  {
    qDebug() << "ZMQ Unsubscribed topic" << QString::fromStdString(topic);

    _zmq_socket.set(zmq::sockopt::unsubscribe, topic);
  }

  _topic_filters.clear();
}
