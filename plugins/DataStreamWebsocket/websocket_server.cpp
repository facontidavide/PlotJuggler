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
#include "websocket_server.h"
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

#include "ui_websocket_server.h"

class WebsocketDialog: public QDialog
{
public:
  WebsocketDialog():
    QDialog(nullptr),
    ui(new Ui::WebSocketDialog)
  {
    ui->setupUi(this);
    ui->lineEditPort->setValidator( new QIntValidator() );
    setWindowTitle("WebSocket Server");

    connect( ui->buttonBox, &QDialogButtonBox::accepted,
             this, &QDialog::accept );
    connect( ui->buttonBox, &QDialogButtonBox::rejected,
             this, &QDialog::reject );
  }
  ~WebsocketDialog()
  {
    while( ui->layoutOptions->count() > 0)
    {
      auto item = ui->layoutOptions->takeAt(0);
      item->widget()->setParent(nullptr);
    }
    delete ui;
  }
  Ui::WebSocketDialog* ui;
};

WebsocketServer::WebsocketServer() :
  _running(false),
  _server("plotJuggler", QWebSocketServer::NonSecureMode)
{
  connect(&_server, &QWebSocketServer::newConnection,
          this, &WebsocketServer::onNewConnection);
}

WebsocketServer::~WebsocketServer()
{
  shutdown();
}

bool WebsocketServer::start(QStringList*)
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

  bool ok = false;

  WebsocketDialog* dialog = new WebsocketDialog();

  for( const auto& it: *availableParsers())
  {
    dialog->ui->comboBoxProtocol->addItem( it.first );

    if(auto widget = it.second->optionsWidget() )
    {
      widget->setVisible(false);
      dialog->ui->layoutOptions->addWidget( widget );
    }
  }

  // load previous values
  QSettings settings;
  QString protocol = settings.value("WebsocketServer::protocol", "JSON").toString();
  int port = settings.value("WebsocketServer::port", 9871).toInt();

  dialog->ui->lineEditPort->setText( QString::number(port) );

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

  dialog->ui->comboBoxProtocol->setCurrentText(protocol);

  int res = dialog->exec();
  if( res == QDialog::Rejected )
  {
    _running = false;
    return false;
  }

  port = dialog->ui->lineEditPort->text().toUShort(&ok);
  protocol = dialog->ui->comboBoxProtocol->currentText();
  dialog->deleteLater();

  _parser = parser_creator->createInstance({}, dataMap());

  // save back to service
  settings.setValue("WebsocketServer::protocol", protocol);
  settings.setValue("WebsocketServer::port", port);

  if ( _server.listen(QHostAddress::Any, port))
  {
    qDebug() << "Websocket listening on port" << port;
    _running = true;
  }
  else
  {
    QMessageBox::warning(nullptr,tr("Websocket Server"),
                         tr("Couldn't open websocket on port %1").arg(port),
                         QMessageBox::Ok);
    _running = false;
  }

  return _running;
}

void WebsocketServer::shutdown()
{
  if (_running)
  {
    socketDisconnected();
    _server.close();
    _running = false;
  }
}

void WebsocketServer::onNewConnection()
{
  QWebSocket* pSocket = _server.nextPendingConnection();
  connect(pSocket, &QWebSocket::textMessageReceived, this, &WebsocketServer::processMessage);
  connect(pSocket, &QWebSocket::disconnected, this, &WebsocketServer::socketDisconnected);
  _clients << pSocket;
}

void WebsocketServer::processMessage(QString message)
{
  std::lock_guard<std::mutex> lock(mutex());

  using namespace std::chrono;
  auto ts = high_resolution_clock::now().time_since_epoch();
  double timestamp = 1e-6* double( duration_cast<microseconds>(ts).count() );

  QByteArray bmsg = message.toLocal8Bit();
  MessageRef msg ( reinterpret_cast<uint8_t*>(bmsg.data()), bmsg.size() );

  try {
    _parser->parseMessage(msg, timestamp);
  } catch (std::exception& err)
  {
    QMessageBox::warning(nullptr,
                         tr("Websocket Server"),
                         tr("Problem parsing the message. Websocket Server will be stopped.\n%1").arg(err.what()),
                         QMessageBox::Ok);
    shutdown();
    emit closed();
    return;
  }
  emit dataReceived();
  return;
}

void WebsocketServer::socketDisconnected()
{
  QWebSocket* pClient = qobject_cast<QWebSocket*>(sender());
  if (pClient)
  {
    disconnect(pClient, &QWebSocket::textMessageReceived, this, &WebsocketServer::processMessage);
    disconnect(pClient, &QWebSocket::disconnected, this, &WebsocketServer::socketDisconnected);
    _clients.removeAll(pClient);
    pClient->deleteLater();
  }
}
