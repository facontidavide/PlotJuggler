#include "datastream_zcm.h"

#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>

#include <iomanip>
#include <iostream>
#include <mutex>

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

using namespace std;
using namespace PJ;

template <typename T>
double toDouble(const void* data)
{
  return static_cast<double>(*reinterpret_cast<const T*>(data));
}

DataStreamZcm::DataStreamZcm() : _subs(nullptr), _running(false)
{
  _dialog = new QDialog;
  _ui = new Ui::DialogZcm;
  _ui->setupUi(_dialog);

  _config_widget = new ConfigZCM("DataStreamZcm", _dialog);
  _ui->mainLayout->insertWidget(5, _config_widget, 1);
}

DataStreamZcm::~DataStreamZcm()
{
  shutdown();
  delete _dialog;
}

const char* DataStreamZcm::name() const
{
  return "Zcm Streamer";
}

bool DataStreamZcm::start(QStringList*)
{
  if (_running)
  {
    return false;
  }

  // We create a Dialog to request the folder to populate zcm::TypeDb
  // and the string to pass to the subscriber.

  // Initialize the lineEdits in the ui with the previous value;
  QSettings settings;
  auto const subscribe_text = settings.value("DataStreamZcm::subscribe", ".*").toString();
  _ui->lineEditSubscribe->setText(subscribe_text);

  {
    auto transport = QString(getenv("ZCM_DEFAULT_URL"));
    transport = settings.value("DataStreamZcm::transport", transport).toString();
    _ui->lineEditTransport->setText(transport);
  }

  // start the dialog and check that OK was pressed
  _dialog->restoreGeometry(settings.value("DataStreamZcm::geometry").toByteArray());
  int res = _dialog->exec();

  settings.setValue("DataStreamZcm::geometry", _dialog->saveGeometry());
  if (res == QDialog::Rejected)
  {
    return false;
  }

  // save the current configuration for the next execution
  settings.setValue("DataStreamZcm::subscribe", _ui->lineEditSubscribe->text());
  settings.setValue("DataStreamZcm::transport", _ui->lineEditTransport->text());

  _transport = _ui->lineEditTransport->text();

  if (!_zcm)
  {
    try
    {
      _zcm.reset(new zcm::ZCM(_transport.toStdString()));
    }
    catch (std::exception& ex)
    {
      QMessageBox::warning(nullptr, "Error",
                           tr("Exception from zcm::ZCM() :\n%1").arg(ex.what()));
      return false;
    }
    if (!_zcm->good())
    {
      QMessageBox::warning(nullptr, "Error", "Failed to create zcm::ZCM()");
      _zcm.reset();
      return false;
    }
  }

  auto libraries = _config_widget->getLibraries();

  // reset the types if it is the first time or folder changed
  if (_types_library != libraries || !_types)
  {
    _types_library = libraries;
    _types.reset(new zcm::TypeDb(_types_library.toStdString()));
    if (!_types->good())
    {
      QMessageBox::warning(nullptr, "Error", "Failed to create zcm::TypeDb()");
      _types.reset();
      return false;
    }
  }

  if (_subscribe_string != _ui->lineEditSubscribe->text() || !_subs)
  {
    _subscribe_string = _ui->lineEditSubscribe->text();
    if (_subs)
    {
      _zcm->unsubscribe(_subs);
    }
    _subs =
        _zcm->subscribe(_subscribe_string.toStdString(), &DataStreamZcm::handler, this);
    if (!_subs)
    {
      QMessageBox::warning(nullptr, "Error", "Failed to subscribe");
      return false;
    }
  }

  _zcm->start();
  _running = true;
  return true;
}

void DataStreamZcm::shutdown()
{
  if (!_running)
  {
    return;
  }
  if (_subs)
  {
    _zcm->unsubscribe(_subs);
    _subs = nullptr;
  }
  _zcm->stop();
  _zcm.reset(nullptr);
  _running = false;
}

bool DataStreamZcm::isRunning() const
{
  return _running;
}

bool DataStreamZcm::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  // RRR (Bendes): Probably also transport string here
  QDomElement elem = doc.createElement("config");
  elem.setAttribute("folder", _types_library);
  elem.setAttribute("subscribe", _subscribe_string);
  elem.setAttribute("transport", _transport);
  parent_element.appendChild(elem);
  return true;
}

bool DataStreamZcm::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("config");
  if (!elem.isNull())
  {
    // RRR (Bendes): Probably also transport string here
    _types_library = elem.attribute("folder");
    _subscribe_string = elem.attribute("subscribe");
    _transport = elem.attribute("transport");
    QSettings settings;
    settings.setValue("DataStreamZcm::folder", _types_library);
    settings.setValue("DataStreamZcm::subscribe", _subscribe_string);
    settings.setValue("DataStreamZcm::transport", _transport);
  }
  return true;
}

void DataStreamZcm::processData(const string& name, zcm_field_type_t type,
                                const void* data, void* usr)
{
  DataStreamZcm* me = (DataStreamZcm*)usr;
  switch (type)
  {
    case ZCM_FIELD_INT8_T:
      me->_numerics.emplace_back(name, toDouble<int8_t>(data));
      break;
    case ZCM_FIELD_INT16_T:
      me->_numerics.emplace_back(name, toDouble<int16_t>(data));
      break;
    case ZCM_FIELD_INT32_T:
      me->_numerics.emplace_back(name, toDouble<int32_t>(data));
      break;
    case ZCM_FIELD_INT64_T:
      me->_numerics.emplace_back(name, toDouble<int64_t>(data));
      break;
    case ZCM_FIELD_BYTE:
      me->_numerics.emplace_back(name, toDouble<uint8_t>(data));
      break;
    case ZCM_FIELD_FLOAT:
      me->_numerics.emplace_back(name, toDouble<float>(data));
      break;
    case ZCM_FIELD_DOUBLE:
      me->_numerics.emplace_back(name, toDouble<double>(data));
      break;
    case ZCM_FIELD_BOOLEAN:
      me->_numerics.emplace_back(name, toDouble<bool>(data));
      break;
    case ZCM_FIELD_STRING:
      me->_strings.emplace_back(name, string(*((const char**)data)));
      break;
    case ZCM_FIELD_USER_TYPE:
      assert(false && "Should not be possble");
  }
};

void DataStreamZcm::handler(const zcm::ReceiveBuffer* rbuf, const string& channel)
{
  zcm::Introspection::processEncodedType(channel, rbuf->data, rbuf->data_size, "/",
                                         *_types.get(), processData, this);
  {
    std::lock_guard<std::mutex> lock(mutex());

    for (auto& n : _numerics)
    {
      auto itr = dataMap().numeric.find(n.first);
      if (itr == dataMap().numeric.end())
      {
        itr = dataMap().addNumeric(n.first);
      }
      itr->second.pushBack({ double(rbuf->recv_utime) / 1e6, n.second });
    }
    for (auto& s : _strings)
    {
      auto itr = dataMap().strings.find(s.first);
      if (itr == dataMap().strings.end())
      {
        itr = dataMap().addStringSeries(s.first);
      }
      itr->second.pushBack({ double(rbuf->recv_utime) / 1e6, s.second });
    }
  }

  emit dataReceived();

  _numerics.clear();
  _strings.clear();
}

void DataStreamZcm::on_pushButtonUrl_clicked()
{
  QString url = getenv("ZCM_DEFAULT_URL");
  if (url.isEmpty())
  {
    QMessageBox::warning(nullptr, "Error",
                         "Environment variable ZCM_DEFAULT_URL not set");
  }
  else
  {
    _ui->lineEditTransport->setText(url);
  }
}
