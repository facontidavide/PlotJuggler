/*DatastreamSerialPort PlotJuggler  Plugin license(Faircode)

Copyright(C) 2026 Valentin Platzgummer
Permission is hereby granted to any person obtaining a copy of this software and
associated documentation files(the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and / or sell copies("Use") of the
Software, and to permit persons to whom the Software is furnished to do so. The
above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS
IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "datastream_serialport.h"

#include <QDebug>
#include <QMessageBox>
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QSettings>

#include <chrono>

#include "PlotJuggler/dialog_utils.h"
#include "ui_datastream_serialport.h"

#define SEPARATOR " - "
#define CHUNK_SIZE 4096

enum FlowControl
{
  FlowControlNone,
  FlowControlSoftware,
  FlowControlHardware,
  FlowControlCount,  // Number of enums ...
};

enum DataBits
{
  DataBits8,
  DataBits7,
  DataBits6,
  DataBits5,
  DataBitsCount,  // Number of enums ...
};

enum Parity
{
  ParityNone,
  ParityEven,
  ParityOdd,
  ParityCount,  // Number of enums ...
};

enum StopBits
{
  StopBits1,
  StopBits2,
  StopBitsCount,  // Number of enums ...
};

static QString enumToString(enum FlowControl flowControl)
{
  switch (flowControl)
  {
    case FlowControlNone:
      return QObject::tr("None");
    case FlowControlSoftware:
      return QObject::tr("Software");
    case FlowControlHardware:
      return QObject::tr("Hardware");
    default:
      Q_ASSERT(0);
      return QObject::tr("?");
  }
}

static QString enumToString(enum DataBits dataBits)
{
  switch (dataBits)
  {
    case DataBits8:
      return QObject::tr("8");
    case DataBits7:
      return QObject::tr("7");
    case DataBits6:
      return QObject::tr("6");
    case DataBits5:
      return QObject::tr("5");
    default:
      Q_ASSERT(0);
      return QObject::tr("?");
  }
}

static QString enumToString(enum Parity parity)
{
  switch (parity)
  {
    case ParityNone:
      return QObject::tr("None");
    case ParityEven:
      return QObject::tr("Even");
    case ParityOdd:
      return QObject::tr("Odd");
    default:
      Q_ASSERT(0);
      return QObject::tr("?");
  }
}

static QString enumToString(enum StopBits stopBits)
{
  switch (stopBits)
  {
    case StopBits1:
      return QObject::tr("1");
    case StopBits2:
      return QObject::tr("2");
    default:
      Q_ASSERT(0);
      return QObject::tr("?");
  }
}

static QSerialPort::Parity convertEnum(enum Parity parity)
{
  switch (parity)
  {
    case ParityNone:
      return QSerialPort::NoParity;
    case ParityEven:
      return QSerialPort::EvenParity;
    case ParityOdd:
      return QSerialPort::OddParity;
    default:
      Q_ASSERT(0);
      return QSerialPort::NoParity;
  }
}

static QSerialPort::StopBits convertEnum(enum StopBits stopBits)
{
  switch (stopBits)
  {
    case StopBits1:
      return QSerialPort::OneStop;
    case StopBits2:
      return QSerialPort::TwoStop;
    default:
      Q_ASSERT(0);
      return QSerialPort::OneStop;
  }
}

static QSerialPort::DataBits convertEnum(enum DataBits bits)
{
  switch (bits)
  {
    case DataBits8:
      return QSerialPort::Data8;
    case DataBits7:
      return QSerialPort::Data7;
    case DataBits6:
      return QSerialPort::Data6;
    case DataBits5:
      return QSerialPort::Data5;
    default:
      Q_ASSERT(0);
      return QSerialPort::Data8;
  }
}

static QSerialPort::FlowControl convertEnum(enum FlowControl flowControl)
{
  switch (flowControl)
  {
    case FlowControlNone:
      return QSerialPort::NoFlowControl;
    case FlowControlSoftware:
      return QSerialPort::SoftwareControl;
    case FlowControlHardware:
      return QSerialPort::HardwareControl;

    default:
      Q_ASSERT(0);
      return QSerialPort::NoFlowControl;
  }
}

static QString serialPortErrorToString(QSerialPort::SerialPortError error)
{
  switch (error)
  {
    case QSerialPort::NoError:
      return QObject::tr("No error occurred.");
    case QSerialPort::DeviceNotFoundError:
      return QObject::tr("An error occurred while attempting to open an non-existing device.");
    case QSerialPort::PermissionError:
      return QObject::tr(
          "An error occurred while attempting to open an already opened device by another process or a user not having enough permission and credentials to open.");
    case QSerialPort::OpenError:
      return QObject::tr("An error occurred while attempting to open an already opened device.");
    case QSerialPort::NotOpenError:
      // This error occurs when an operation is executed that can only be successfully performed
      // if the device is open. This value was introduced in QtSerialPort 5.2.
      return QObject::tr("Internal Error");
    case QSerialPort::WriteError:
      return QObject::tr("An I/O error occurred while writing the data.");
    case QSerialPort::ReadError:
      return QObject::tr("An I/O error occurred while reading the data.");
    case QSerialPort::ResourceError:
      return QObject::tr("An I/O error occurred.");
    case QSerialPort::UnsupportedOperationError:
      return QObject::tr(
          "The requested device operation is not supported or prohibited by the operating system.");
    case QSerialPort::TimeoutError:
      return QObject::tr("A timeout error occurred.");

    default:
    case QSerialPort::UnknownError:
      return QObject::tr("An unidentified error occurred.");
  }
}

class MsgSplitter
{
public:
  // Split data into messages that can be digested by a parser. Returns a list of messages.
  // The list may be empty if no message was found, or data didn't contain the entire messages.
  // A stream of messages can be fed chunk wise to this function.
  virtual QList<QByteArray> process(const QByteArray& data) = 0;
};

class JSONSplitter : public MsgSplitter
{
public:
  // Looks for JSON objects within data. A json object may be preceded by arbitrary characters which
  // are removed. The json object may not contain nested objects.
  QList<QByteArray> process(const QByteArray& data)
  {
    QList<QByteArray> res;

    for (const auto byte : data)
    {
      if (buf.count() > 1024 * 1024)
      {
        buf.clear();  // limit memory usage to 1MB.
      }

      if (byte == '{')
      {
        start_found = true;
        buf.clear();
      }
      else if (byte == '}')
      {
        if (start_found)
        {
          buf.append(byte);
          res.append(buf);
          buf.clear();
          start_found = false;
        }
      }

      if (start_found)
      {
        buf.append(byte);
      }
    }

    return res;
  }

private:
  QByteArray buf{};
  bool start_found{ false };
};

class DatastreamSerialPortDialog : public QDialog
{
public:
  DatastreamSerialPortDialog() : QDialog(nullptr), ui(new Ui::SerialPortDialog)
  {
    ui->setupUi(this);
    setWindowTitle("Serial Port");

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  }
  ~DatastreamSerialPortDialog()
  {
    while (ui->layoutOptions->count() > 0)
    {
      auto item = ui->layoutOptions->takeAt(0);
      item->widget()->setParent(nullptr);
    }
    delete ui;
  }
  Ui::SerialPortDialog* ui;
};

DatastreamSerialPort::DatastreamSerialPort()
  : _running(false), _serialPort(nullptr), _splitter(nullptr)
{
}

DatastreamSerialPort::~DatastreamSerialPort()
{
  shutdown();
}

bool DatastreamSerialPort::start(QStringList*)
{
  if (_running)
  {
    return _running;
  }

  if (parserFactories() == nullptr || parserFactories()->empty())
  {
    QMessageBox::warning(nullptr, tr("Serial Port"), tr("No available MessageParsers"),
                         QMessageBox::Ok);
    _running = false;
    return false;
  }

  DatastreamSerialPortDialog dialog;

  // Add protocol option widgets.
  QStringList supportedProtocols = { "json" };
  for (const auto& it : *parserFactories())
  {
    if (!supportedProtocols.contains(it.first))
    {
      continue;
    }
    dialog.ui->comboBoxProtocol->addItem(it.first);

    if (auto widget = it.second->optionsWidget())
    {
      widget->setVisible(false);
      dialog.ui->layoutOptions->addWidget(widget);
    }
  }

  // Populate combo boxes
  for (int i = 0; i < ParityCount; ++i)
  {
    dialog.ui->comboBoxParity->addItem(enumToString(Parity(i)));
  }
  for (int i = 0; i < StopBitsCount; ++i)
  {
    dialog.ui->comboBoxStopBits->addItem(enumToString(StopBits(i)));
  }
  for (int i = 0; i < FlowControlCount; ++i)
  {
    dialog.ui->comboBoxFlowControl->addItem(enumToString(FlowControl(i)));
  }
  for (int i = 0; i < DataBitsCount; ++i)
  {
    dialog.ui->comboBoxDataBits->addItem(enumToString(DataBits(i)));
  }
  for (qint32 baudrate : QSerialPortInfo::standardBaudRates())
  {
    dialog.ui->comboBoxBaudRate->addItem(QString("%1").arg(baudrate));
  }

  auto refreshPortList = [&dialog]() {
    dialog.ui->comboBoxPort->clear();
    Q_FOREACH (QSerialPortInfo port, QSerialPortInfo::availablePorts())
    {
      QString portName = port.portName();
      QString manufacturer = port.manufacturer();
      if (manufacturer.size() > 0)
      {
        portName += SEPARATOR + manufacturer;
      }
      QString description = port.description();
      if (description.size() > 0)
      {
        portName += SEPARATOR + description;
      }
      dialog.ui->comboBoxPort->addItem(portName);
    }
  };
  refreshPortList();
  connect(dialog.ui->pushButtonRefresh, &QPushButton::clicked, this, refreshPortList);

  ParserFactoryPlugin::Ptr parser_creator;
  auto onComboChanged = [this, &dialog, &parser_creator](const QString& selected_protocol) {
    if (parser_creator)
    {
      if (auto prev_widget = parser_creator->optionsWidget())
      {
        prev_widget->setVisible(false);
      }
    }
    parser_creator = parserFactories()->at(selected_protocol);

    showOptionsWidget(&dialog, dialog.ui->boxOptions, parser_creator->optionsWidget());
  };
  onComboChanged("json");
  connect(dialog.ui->comboBoxProtocol, qOverload<const QString&>(&QComboBox::currentIndexChanged),
          this, onComboChanged);

  // load previous values
  QSettings settings;
  QString portDescription = settings.value("DatastreamSerialPort::port", "/dev/ttyACM0").toString();
  int baudrate = settings.value("DatastreamSerialPort::baud-rate", 115200).toInt();
  enum Parity parity = Parity(settings.value("DatastreamSerialPort::parity", ParityNone).toInt());
  enum DataBits bits = DataBits(settings.value("DatastreamSerialPort::bits", DataBits8).toInt());
  enum StopBits stopBits =
      StopBits(settings.value("DatastreamSerialPort::stop-bits", StopBits1).toInt());
  enum FlowControl flowControl =
      FlowControl(settings.value("DatastreamSerialPort::flow-control", FlowControlNone).toInt());
  QString protocol = settings.value("DatastreamSerialPort::protocol").toString();
  if (parserFactories()->find(protocol) == parserFactories()->end())
  {
    protocol = "json";
  }

  dialog.ui->comboBoxPort->setCurrentText(portDescription);
  dialog.ui->comboBoxBaudRate->setCurrentText(QString("%1").arg(baudrate));
  dialog.ui->comboBoxParity->setCurrentIndex(parity);
  dialog.ui->comboBoxDataBits->setCurrentIndex(bits);
  dialog.ui->comboBoxStopBits->setCurrentIndex(stopBits);
  dialog.ui->comboBoxFlowControl->setCurrentIndex(flowControl);
  dialog.ui->comboBoxProtocol->setCurrentText(protocol);

  int res = dialog.exec();

  portDescription = dialog.ui->comboBoxPort->currentText();
  baudrate = dialog.ui->comboBoxBaudRate->currentText().toInt();
  parity = Parity(dialog.ui->comboBoxParity->currentIndex());
  bits = DataBits(dialog.ui->comboBoxDataBits->currentIndex());
  stopBits = StopBits(dialog.ui->comboBoxStopBits->currentIndex());
  flowControl = FlowControl(dialog.ui->comboBoxFlowControl->currentIndex());
  protocol = dialog.ui->comboBoxProtocol->currentText();

  // save back values
  settings.setValue("DatastreamSerialPort::port", portDescription);
  settings.setValue("DatastreamSerialPort::baud-rate", baudrate);
  settings.setValue("DatastreamSerialPort::parity", parity);
  settings.setValue("DatastreamSerialPort::bits", bits);
  settings.setValue("DatastreamSerialPort::stop-bits", stopBits);
  settings.setValue("DatastreamSerialPort::flow-control", flowControl);
  settings.setValue("DatastreamSerialPort::protocol", protocol);

  if (res == QDialog::Rejected)
  {
    _running = false;
    return false;
  }

  Q_ASSERT(!_splitter);
  if (protocol == "json")
  {
    _splitter = new JSONSplitter();
  }

  _parser = parser_creator->createParser({}, {}, {}, dataMap());

  // portDescription == "device_name - manufacturer - description", assuming SEPARATOR == " - "
  auto tokens = portDescription.split(SEPARATOR);
  Q_ASSERT(tokens.size() > 0);
  QString portName = tokens[0];

  Q_ASSERT(!_serialPort);
  _serialPort = new QSerialPort(this);
  _serialPort->setPortName(portName);
  _serialPort->setBaudRate(baudrate);
  _serialPort->setParity(convertEnum(parity));
  _serialPort->setFlowControl(convertEnum(flowControl));
  _serialPort->setDataBits(convertEnum(bits));
  _serialPort->setStopBits(convertEnum(stopBits));

  if (_serialPort->open(QIODevice::ReadOnly))
  {
    qDebug() << tr("Serial port opened \"%1\"").arg(portDescription);

    _running = true;
    connect(_serialPort, &QSerialPort::readyRead, this, &DatastreamSerialPort::processMessage);
  }
  else
  {
    QString reason = serialPortErrorToString(_serialPort->error());
    QMessageBox::warning(
        nullptr, tr("Serial Port"),
        tr("Couldn't open serial port \"%1\": %2").arg(portDescription).arg(reason),
        QMessageBox::Ok);
    shutdown();
  }

  return _running;
}

void DatastreamSerialPort::shutdown()
{
  if (_running)
  {
    _running = false;
  }
  if (_serialPort)
  {
    _serialPort->close();
    _serialPort->deleteLater();
    _serialPort = nullptr;
  }
  if (_splitter)
  {
    delete _splitter;
    _splitter = nullptr;
  }
}

void DatastreamSerialPort::processMessage()
{
  if (!_running || !_serialPort || !_splitter)
  {
    return;
  }

  while (_serialPort->bytesAvailable() > 0)
  {
    using namespace std::chrono;
    auto ts = high_resolution_clock::now().time_since_epoch();
    double timestamp = 1e-6 * double(duration_cast<microseconds>(ts).count());

    auto msgList = _splitter->process(_serialPort->read(CHUNK_SIZE));
    if (msgList.count() == 0)
    {
      return;
    }

    for (const auto& data : msgList)
    {
      MessageRef msg(reinterpret_cast<const uint8_t*>(data.data()), data.count());
      try
      {
        std::lock_guard<std::mutex> lock(mutex());
        // important use the mutex to protect any access to the data
        _parser->parseMessage(msg, timestamp);
      }
      catch (std::exception& err)
      {
        qDebug() << tr("parser error, data:") << data;
        QMessageBox::warning(nullptr, tr("Serial Port"),
                             tr("Problem parsing the message. Serial port will be "
                                "closed.\n%1")
                                 .arg(err.what()),
                             QMessageBox::Ok);
        shutdown();
        // notify the GUI
        emit closed();
        return;
      }
    }
  }

  // notify the GUI
  emit dataReceived();
  return;
}
