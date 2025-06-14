/*Wensocket PlotJuggler Plugin license(Faircode, Davide Faconti)

Copyright(C) 2018 Philippe Gauthier - ISIR - UPMC
Copyright(C) 2020 Davide Faconti
Permission is hereby granted to any person obtaining a copy of this software and
associated documentation files(the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and / or sell copies("Use") of the Software, and to permit persons
to whom the Software is furnished to do so. The above copyright notice and this permission
notice shall be included in all copies or substantial portions of the Software. THE
SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include "udp_server.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QDialog>
#include <mutex>
#include <QWebSocket>
#include <QIntValidator>
#include <QMessageBox>
#include <chrono>
#include <QNetworkDatagram>
#include <QNetworkInterface>

#include "ui_udp_server.h"

class UdpServerDialog : public QDialog
{
public:
  UdpServerDialog() : QDialog(nullptr), ui(new Ui::UDPServerDialog)
  {
    ui->setupUi(this);
    ui->lineEditPort->setValidator(new QIntValidator());
    setWindowTitle("UDP Server");

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  }
  ~UdpServerDialog()
  {
    while (ui->layoutOptions->count() > 0)
    {
      auto item = ui->layoutOptions->takeAt(0);
      item->widget()->setParent(nullptr);
    }
    delete ui;
  }
  Ui::UDPServerDialog* ui;
};

UDP_Server::UDP_Server() : _running(false)
{
}

UDP_Server::~UDP_Server()
{
  shutdown();
}

bool UDP_Server::start(QStringList*)
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

  UdpServerDialog dialog;

  for (const auto& it : *parserFactories())
  {
    dialog.ui->comboBoxProtocol->addItem(it.first);

    if (auto widget = it.second->optionsWidget())
    {
      widget->setVisible(false);
      dialog.ui->layoutOptions->addWidget(widget);
    }
  }

  // load previous values
  QSettings settings;
  QString address_str = settings.value("UDP_Server::address", "127.0.0.1").toString();
  int port = settings.value("UDP_Server::port", 9870).toInt();
  QString protocol = settings.value("UDP_Server::protocol").toString();
  if (parserFactories()->find(protocol) == parserFactories()->end())
  {
    protocol = "json";
  }

  dialog.ui->lineEditAddress->setText(address_str);
  dialog.ui->lineEditPort->setText(QString::number(port));

  ParserFactoryPlugin::Ptr parser_creator;

  auto onComboChanged = [&](const QString& selected_protocol) {
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
  };

  connect(dialog.ui->comboBoxProtocol,
          qOverload<const QString&>(&QComboBox::currentIndexChanged), this,
          onComboChanged);

  dialog.ui->comboBoxProtocol->setCurrentText(protocol);
  onComboChanged(protocol);

  int res = dialog.exec();
  if (res == QDialog::Rejected)
  {
    _running = false;
    return false;
  }

  address_str = dialog.ui->lineEditAddress->text();
  port = dialog.ui->lineEditPort->text().toUShort(&ok);
  protocol = dialog.ui->comboBoxProtocol->currentText();

  _parser = parser_creator->createParser({}, {}, {}, dataMap());

  // save back to service
  settings.setValue("UDP_Server::protocol", protocol);
  settings.setValue("UDP_Server::address", address_str);
  settings.setValue("UDP_Server::port", port);

  QHostAddress address(address_str);

  bool success = true;
  success &= !address.isNull();

  _udp_socket = new QUdpSocket();
  int ip_version = 4;
  if (address.protocol() == QAbstractSocket::IPv6Protocol)
  {
    ip_version = 6;
  }

  if (!address.isMulticast())
  {
    success &= _udp_socket->bind(address, port);
  }
  else
  {
    QHostAddress bind_address = address;
    if (ip_version == 6)
    {
      // IPv6 multicast needs to bind to AnyIPv6
      bind_address = QHostAddress::AnyIPv6;
    }
    success &= _udp_socket->bind(bind_address, port,
                                 QAbstractSocket::ShareAddress |
                                     QAbstractSocket::ReuseAddressHint);
    if (!success)
    {
      qDebug() << tr("Couldn't bind IPv%3 UDP socket (%1, %2)")
                      .arg(address_str)
                      .arg(port)
                      .arg(ip_version);
    }

    // Add multicast group membership to all interfaces which support multicast.
    bool bound_one_interface = false;
    for (const auto& interface : QNetworkInterface::allInterfaces())
    {
      QNetworkInterface::InterfaceFlags iflags = interface.flags();
      if (success && interface.isValid() &&
          !iflags.testFlag(QNetworkInterface::IsLoopBack) &&
          iflags.testFlag(QNetworkInterface::CanMulticast) &&
          iflags.testFlag(QNetworkInterface::IsRunning))
      {
        if (_udp_socket->joinMulticastGroup(address, interface))
        {
          bound_one_interface = true;
        }
        else
        {
          qDebug() << tr("Couldn't join IPv%4 multicast group (%1, %2) on interface %3")
                          .arg(address_str)
                          .arg(port)
                          .arg(interface.name().arg(ip_version));
        }
      }
    }
    success &= bound_one_interface;
  }

  _running = true;

  connect(_udp_socket, &QUdpSocket::readyRead, this, &UDP_Server::processMessage);

  if (success)
  {
    qDebug() << tr("IPv%3 UDP listening on (%1, %2)")
                    .arg(address_str)
                    .arg(port)
                    .arg(ip_version);
  }
  else
  {
    QMessageBox::warning(nullptr, tr("UDP Server"),
                         tr("Couldn't bind to IPv%4 UDP server at (%1, %2)")
                             .arg(address_str)
                             .arg(port)
                             .arg(ip_version),
                         QMessageBox::Ok);
    shutdown();
  }

  return _running;
}

void UDP_Server::shutdown()
{
  if (_running && _udp_socket)
  {
    _udp_socket->deleteLater();
    _running = false;
  }
}

void UDP_Server::processMessage()
{
  while (_udp_socket->hasPendingDatagrams())
  {
    QNetworkDatagram datagram = _udp_socket->receiveDatagram();

    using namespace std::chrono;
    auto ts = high_resolution_clock::now().time_since_epoch();
    double timestamp = 1e-6 * double(duration_cast<microseconds>(ts).count());

    QByteArray m = datagram.data();
    MessageRef msg(reinterpret_cast<uint8_t*>(m.data()), m.count());

    try
    {
      std::lock_guard<std::mutex> lock(mutex());
      // important use the mutex to protect any access to the data
      _parser->parseMessage(msg, timestamp);
    }
    catch (std::exception& err)
    {
      QMessageBox::warning(nullptr, tr("UDP Server"),
                           tr("Problem parsing the message. UDP Server will be "
                              "stopped.\n%1")
                               .arg(err.what()),
                           QMessageBox::Ok);
      shutdown();
      // notify the GUI
      emit closed();
      return;
    }
  }
  // notify the GUI
  emit dataReceived();
  return;
}
