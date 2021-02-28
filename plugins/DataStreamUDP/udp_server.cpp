/*Wensocket PlotJuggler Plugin license(Faircode, Davide Faconti)

Copyright(C) 2018 Philippe Gauthier - ISIR - UPMC
Copyright(C) 2020 Davide Faconti
Permission is hereby granted to any person obtaining a copy of this software and associated documentation files(the
"Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and / or sell copies("Use") of the Software, and to permit persons to whom the
Software is furnished to do so. The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
OR OTHER DEALINGS IN THE SOFTWARE.
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

#include "ui_udp_server.h"

class UdpServerDialog: public QDialog
{
public:
  UdpServerDialog():
    QDialog(nullptr),
    ui(new Ui::UDPServerDialog)
  {
    ui->setupUi(this);
    ui->lineEditPort->setValidator( new QIntValidator() );
    setWindowTitle("UDP Server");

    connect( ui->buttonBox, &QDialogButtonBox::accepted,
             this, &QDialog::accept );
    connect( ui->buttonBox, &QDialogButtonBox::rejected,
             this, &QDialog::reject );
  }
  ~UdpServerDialog()
  {
    while( ui->layoutOptions->count() > 0)
    {
      auto item = ui->layoutOptions->takeAt(0);
      item->widget()->setParent(nullptr);
    }
    delete ui;
  }
  Ui::UDPServerDialog* ui;
};

UDP_Server::UDP_Server() :
  _running(false)
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

  if( !availableParsers() )
  {
    QMessageBox::warning(nullptr,tr("UDP Server"), tr("No available MessageParsers"),  QMessageBox::Ok);
    _running = false;
    return false;
  }

  bool ok = false;

  UdpServerDialog dialog;

  for( const auto& it: *availableParsers())
  {
    dialog.ui->comboBoxProtocol->addItem( it.first );

    if(auto widget = it.second->optionsWidget() )
    {
      widget->setVisible(false);
      dialog.ui->layoutOptions->addWidget( widget );
    }
  }

  // load previous values
  QSettings settings;
  QString protocol = settings.value("UDP_Server::protocol", "JSON").toString();
  int port = settings.value("UDP_Server::port", 9870).toInt();

  dialog.ui->lineEditPort->setText( QString::number(port) );

  std::shared_ptr<MessageParserCreator> parser_creator;

  connect(dialog.ui->comboBoxProtocol, qOverload<int>( &QComboBox::currentIndexChanged), this,
          [&](int)
  {
    if( parser_creator ){
      QWidget*  prev_widget = parser_creator->optionsWidget();
      prev_widget->setVisible(false);
    }
    parser_creator = availableParsers()->at( protocol );

    if(auto widget = parser_creator->optionsWidget() ){
      widget->setVisible(true);
    }
  });

  dialog.ui->comboBoxProtocol->setCurrentText(protocol);

  int res = dialog.exec();
  if( res == QDialog::Rejected )
  {
    _running = false;
    return false;
  }

  port = dialog.ui->lineEditPort->text().toUShort(&ok);
  protocol = dialog.ui->comboBoxProtocol->currentText();

  _parser = parser_creator->createInstance({}, dataMap());

  // save back to service
  settings.setValue("UDP_Server::protocol", protocol);
  settings.setValue("UDP_Server::port", port);

  _udp_socket = new QUdpSocket();
  _udp_socket->bind(QHostAddress::LocalHost, port);

  connect(_udp_socket, &QUdpSocket::readyRead,
          this, &UDP_Server::processMessage);

  if ( _udp_socket )
  {
    qDebug() << "UDP listening on port" << port;
    _running = true;
  }
  else
  {
    QMessageBox::warning(nullptr,tr("UDP Server"),
                         tr("Couldn't bind UDP port %1").arg(port),
                         QMessageBox::Ok);
    _running = false;
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
  while (_udp_socket->hasPendingDatagrams()) {

    QNetworkDatagram datagram = _udp_socket->receiveDatagram();


    using namespace std::chrono;
    auto ts = high_resolution_clock::now().time_since_epoch();
    double timestamp = 1e-6* double( duration_cast<microseconds>(ts).count() );

    QByteArray m = datagram.data();
    MessageRef msg ( reinterpret_cast<uint8_t*>(m.data()), m.count() );    

    try {
      std::lock_guard<std::mutex> lock(mutex());
      // important use the mutex to protect any access to the data
      _parser->parseMessage(msg, timestamp);
    } catch (std::exception& err)
    {
      QMessageBox::warning(nullptr,
                           tr("UDP Server"),
                           tr("Problem parsing the message. UDP Server will be stopped.\n%1").arg(err.what()),
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

