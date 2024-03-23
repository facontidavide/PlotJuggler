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
#include <QIntValidator>
#include <QMessageBox>
#include <chrono>
#include <arpa/inet.h>
#include <unistd.h>

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
  QString protocol = settings.value("UDP_Server::protocol", "JSON").toString();
  QString address_str = settings.value("UDP_Server::address", "127.0.0.1").toString();
  int port = settings.value("UDP_Server::port", 9870).toInt();

  dialog.ui->lineEditAddress->setText(address_str);
  dialog.ui->lineEditPort->setText(QString::number(port));

  ParserFactoryPlugin::Ptr parser_creator;

  connect(dialog.ui->comboBoxProtocol,
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

  dialog.ui->comboBoxProtocol->setCurrentText(protocol);

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

  // this->startAsync(address,port,address_str);
  workerThread = new WorkerThread(this);
  workerThread->udpServer = this;
  workerThread->port = port;
  workerThread->address_str = address_str;
  workerThread->start();

  while (!_threadStarted)
  {
    std::this_thread::sleep_until(std::chrono::high_resolution_clock::now() +
                                  std::chrono::milliseconds(10));
  }

  return _running;
}

void UDP_Server::shutdown()
{
  if (_running)
  {
    _running = false;
    while (!_threadFinished)
    {
      std::this_thread::sleep_until(std::chrono::high_resolution_clock::now() +
                                    std::chrono::milliseconds(10));
    }
    workerThread->deleteLater();
    _threadStarted = false;
  }
}

int UDP_Server::normal_socket(char* address_str, int port)
{
  bool success = true;

  // UDP socket setup
  int sockfd;
  struct sockaddr_in servaddr;

  // Create socket
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    success = false;
    perror("socket creation failed");
  }

  memset(&servaddr, 0, sizeof(servaddr));

  // Server address setup
  servaddr.sin_family = AF_INET;
  // network interface
  if (inet_aton(address_str, &servaddr.sin_addr) == 0)
  {
    success = false;
    perror("Invalid address");
  }
  servaddr.sin_port = htons(port);  // Port 9870 (adjust as needed)

  // Bind socket with server address
  if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
  {
    success = false;
    perror("bind failed");
  }

  int n;
  // Receive data from client
  _running = true;

  if (success)
  {
    qDebug() << tr("UDP listening on (%1, %2)").arg(address_str).arg(port);
  }
  else
  {
    qDebug() << tr("Couldn't bind to UDP (%1, %2)").arg(address_str).arg(port);
    QMessageBox::warning(nullptr, tr("UDP Server"),
                         tr("Couldn't bind to UDP (%1, %2)").arg(address_str).arg(port),
                         QMessageBox::Ok);
    close(sockfd);
    shutdown();
    return -1;
  }

  _threadStarted = true;
  while (_running)
  {
    std::vector<unsigned char> data(2048);
    n = recvfrom(sockfd, data.data(), 2048, 0, NULL, 0);
    if (n < 0)
    {
      perror("recvfrom failed");
      close(sockfd);
      shutdown();
      return -1;
    }

    using namespace std::chrono;
    auto ts = high_resolution_clock::now().time_since_epoch();
    double timestamp = 1e-6 * double(duration_cast<microseconds>(ts).count());
    MessageRef msg(reinterpret_cast<uint8_t*>(data.data()), n);
    // printf("%s\n", data.data());
    try
    {
      std::lock_guard<std::mutex> lock(mutex());
      // important use the mutex to protect any access to the data
      _parser->parseMessage(msg, timestamp);

      // notify the GUI
      emit dataReceived();
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
      return -1;
    }
  }
  qDebug() << tr("Closing UDP thread");
  close(sockfd);
  _threadFinished = true;
  return 1;
}