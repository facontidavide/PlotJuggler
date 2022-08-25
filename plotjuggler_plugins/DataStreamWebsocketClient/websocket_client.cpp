/*Websocket PlotJuggler Plugin license(Faircode, Davide Faconti)

Copyright(C) 2018 Philippe Gauthier - ISIR - UPMC
Copyright(C) 2020 Davide Faconti
Copyright(C) 2022 Thijs Gloudemans
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
#include "websocket_client.h"
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

#include "ui_websocket_client.h"

class WebsocketClientDialog : public QDialog
{
public:
  WebsocketClientDialog() : QDialog(nullptr), ui(new Ui::WebsocketClientDialog)
  {
    ui->setupUi(this);
    setWindowTitle("WebSocket Client");

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  }
  ~WebsocketClientDialog()
  {
    while (ui->layoutOptions->count() > 0)
    {
      auto item = ui->layoutOptions->takeAt(0);
      item->widget()->setParent(nullptr);
    }
    delete ui;
  }
  Ui::WebsocketClientDialog* ui;
};

WebsocketClient::WebsocketClient(): 
  _running(false), _socket("plotJuggler", QWebSocketProtocol::VersionUnknown)
{
  connect(&_socket, &QWebSocket::connected, this, &WebsocketClient::onNewConnection);
  // connect(&_socket, &QWebSocket::disconnected, this, &WebsocketClient::socketDisconnected);
}

WebsocketClient::~WebsocketClient()
{
  shutdown();
}

bool WebsocketClient::start(QStringList*)
{
  if (_running)
  {
    return _running;
  }

  if (parserFactories() == nullptr || parserFactories()->empty())
  {
    QMessageBox::warning(nullptr, tr("Websocket Client"),
                         tr("No available MessageParsers"), QMessageBox::Ok);
    _running = false;
    return false;
  }

  bool ok = false;

  WebsocketClientDialog* dialog = new WebsocketClientDialog();

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
  QString protocol = settings.value("WebsocketClient::protocol", "JSON").toString();
  QString url = settings.value("WebsocketClient::url", "0.0.0.0:9871").toString();

  dialog->ui->lineEditUrl->setText(url);

  ParserFactoryPlugin::Ptr parser_creator;

  connect(dialog->ui->comboBoxProtocol,
          qOverload<const QString &>(&QComboBox::currentIndexChanged),
          this, [&](const QString & selected_protocol) {
            if (parser_creator)
            {
              if( auto prev_widget = parser_creator->optionsWidget())
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

  url = dialog->ui->lineEditUrl->text();
  protocol = dialog->ui->comboBoxProtocol->currentText();
  dialog->deleteLater();

  QString socket_address = cleanQString(url);
  _parser = parser_creator->createParser({}, {}, {}, dataMap());

  // save back to service
  settings.setValue("WebsocketClient::protocol", protocol);
  settings.setValue("WebsocketClient::url", url);

  _socket.open(QUrl(socket_address));
  // if (_socket.isValid())
  // {
  //   qDebug() << "Websocket is ready for reading" << url;
  //   _running = true;
  // }
  // else
  // {
  //   QMessageBox::warning(nullptr, tr("Websocket Client"),
  //                        tr("Websocket with url %1 is invalid.").arg(url),
  //                        QMessageBox::Ok);
  //   _running = false;
  // }

  _running = true;

  return _running;
}

void WebsocketClient::shutdown()
{
  if (_running)
  {
    socketDisconnected();
    qDebug() << "WebSocket disconnected.";
    _socket.close();
    _running = false;
  }
}

void WebsocketClient::onNewConnection() // handle_new_connection
{
  connect(&_socket, &QWebSocket::textMessageReceived, this,
          &WebsocketClient::processMessage);
  connect(&_socket, &QWebSocket::disconnected, this, &WebsocketClient::socketDisconnected);
}

void WebsocketClient::processMessage(QString message) 
{
  std::lock_guard<std::mutex> lock(mutex());
  using namespace std::chrono;
  auto ts = high_resolution_clock::now().time_since_epoch();
  double timestamp = 1e-6 * double(duration_cast<microseconds>(ts).count());
  QByteArray bmsg = message.toLocal8Bit();
  MessageRef msg(reinterpret_cast<uint8_t*>(bmsg.data()), bmsg.size());

  try
  {
    _parser->parseMessage(msg, timestamp);
  }
  catch (std::exception& err)
  {
    QMessageBox::warning(nullptr, tr("Websocket client"),
                         tr("Problem parsing the message. Websocket client will be "
                            "stopped.\n%1")
                             .arg(err.what()),
                         QMessageBox::Ok);
    shutdown();
    emit closed();
    return;
  }
  emit dataReceived();
  return;
}

void WebsocketClient::socketDisconnected() 
{
}

QString WebsocketClient::cleanQString(QString to_clean)
{
  QString cleaned;

  for(int i = 0; i < to_clean.size(); ++i) 
  {
    QChar qc = to_clean.at(i);
    unsigned char c = *(unsigned char*)(&qc);
    if(c >= 127) 
    {
      cleaned.append("?");
    } else if (QChar(c).isPrint()) {
      cleaned.append(QChar(c));
    }
  }

  return cleaned;
}
   