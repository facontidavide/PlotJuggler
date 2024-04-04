#include "dataload_zcm.h"

#include <QDebug>
#include <QFile>
#include <QInputDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QTextStream>
#include <QWidget>
#include <QFileDialog>

#include <iostream>

#include <zcm/zcm-cpp.hpp>
#include <zcm/tools/Introspection.hpp>

using namespace std;

static bool verbose = false;

DataLoadZcm::DataLoadZcm()
{
  _dialog = new QDialog();
  _ui = new Ui::DialogZcm();
  _ui->setupUi(_dialog);

  _config_widget = new ConfigZCM("DataLoadZcm", _dialog);
  _ui->mainLayout->insertWidget(0, _config_widget, 1);

  _ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

  _ui->listWidgetChannels->setSelectionMode(QAbstractItemView::ExtendedSelection);

  connect(_ui->listWidgetChannels, &QListWidget::itemSelectionChanged, this, [this]() {
    auto selected = _ui->listWidgetChannels->selectionModel()->selectedIndexes();
    bool box_enabled = selected.size() > 0;
    _ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(box_enabled);
  });
}

DataLoadZcm::~DataLoadZcm()
{
  delete _dialog;
}

const char* DataLoadZcm::name() const
{
  return "DataLoad Zcm";
}

const vector<const char*>& DataLoadZcm::compatibleFileExtensions() const
{
  static vector<const char*> extensions = { "zcmlog" };
  return extensions;
}

static int processInputLog(const string& logpath,
                           function<void(const zcm::LogEvent* evt)> processEvent)
{
  zcm::LogFile inlog(logpath, "r");
  if (!inlog.good())
  {
    cerr << "Unable to open input zcm log: " << logpath << endl;
    return 1;
  }

  auto processLog = [&inlog](function<void(const zcm::LogEvent* evt)> processEvent) {
    const zcm::LogEvent* evt;
    off64_t offset;
    static int lastPrintPercent = 0;

    fseeko(inlog.getFilePtr(), 0, SEEK_END);
    off64_t logSize = ftello(inlog.getFilePtr());
    fseeko(inlog.getFilePtr(), 0, SEEK_SET);

    QProgressDialog progress_dialog;
    progress_dialog.setLabelText("Loading... please wait");
    progress_dialog.setWindowModality(Qt::ApplicationModal);
    progress_dialog.setRange(0, 100);
    progress_dialog.setAutoClose(true);
    progress_dialog.setAutoReset(true);
    progress_dialog.show();

    bool interrupted = false;

    while (1)
    {
      offset = ftello(inlog.getFilePtr());

      int percent = 100.0 * offset / (logSize == 0 ? 1 : logSize);
      if (percent != lastPrintPercent)
      {
        if (verbose)
        {
          cout << "\r"
               << "Percent Complete: " << percent << flush;
        }
        lastPrintPercent = percent;

        progress_dialog.setValue(percent);
        if (progress_dialog.wasCanceled())
        {
          interrupted = true;
          break;
        }
      }

      evt = inlog.readNextEvent();
      if (evt == nullptr)
        break;

      processEvent(evt);
    }
    if (verbose)
    {
      if (lastPrintPercent != 100 && !interrupted)
        cout << "\r"
             << "Percent Complete: 100" << flush;
      cout << endl;
    }

    if (interrupted)
      progress_dialog.cancel();
  };

  processLog(processEvent);

  inlog.close();

  return 0;
}

bool DataLoadZcm::refreshChannels(const string& filepath)
{
  _all_channels.clear();
  _all_channels_filepath = filepath;

  auto processEvent = [&](const zcm::LogEvent* evt) {
    _all_channels.insert(evt->channel);
  };

  return processInputLog(filepath, processEvent) == 0;
}

bool DataLoadZcm::launchDialog(const string& filepath)
{
  QSettings settings;
  _dialog->restoreGeometry(settings.value("DataLoadZcm.geometry").toByteArray());

  _ui->listWidgetChannels->clear();
  for (auto& c : _all_channels)
  {
    auto chan = QString::fromStdString(c);
    _ui->listWidgetChannels->addItem(chan);
  }
  _ui->listWidgetChannels->sortItems();

  auto selected_channels = settings.value("DataLoadZcm.selected_channels").toStringList();
  for (int row = 0; row < _ui->listWidgetChannels->count(); row++)
  {
    auto item = _ui->listWidgetChannels->item(row);
    if (selected_channels.contains(item->text()))
    {
      item->setSelected(true);
    }
  }
  selected_channels.clear();

  int res = _dialog->exec();
  settings.setValue("DataLoadZcm.geometry", _dialog->saveGeometry());

  if (res == QDialog::Rejected)
  {
    return false;
  }

  _selected_channels.clear();

  QModelIndexList indexes = _ui->listWidgetChannels->selectionModel()->selectedRows();
  for (auto& i : indexes)
  {
    auto item = _ui->listWidgetChannels->item(i.row());
    _selected_channels.insert(item->text().toStdString());
    selected_channels.push_back(item->text());
  }

  settings.setValue("DataLoadZcm.selected_channels", selected_channels);

  return !indexes.empty();
}

template <typename T>
double toDouble(const void* data)
{
  return static_cast<double>(*reinterpret_cast<const T*>(data));
}

struct ProcessUsr
{
  vector<pair<string, double>>& numerics;
  vector<pair<string, string>>& strings;
};
static void processData(const string& name, zcm_field_type_t type, const void* data,
                        void* usr)
{
  ProcessUsr* v = (ProcessUsr*)usr;
  switch (type)
  {
    case ZCM_FIELD_INT8_T:
      v->numerics.emplace_back(name, toDouble<int8_t>(data));
      break;
    case ZCM_FIELD_INT16_T:
      v->numerics.emplace_back(name, toDouble<int16_t>(data));
      break;
    case ZCM_FIELD_INT32_T:
      v->numerics.emplace_back(name, toDouble<int32_t>(data));
      break;
    case ZCM_FIELD_INT64_T:
      v->numerics.emplace_back(name, toDouble<int64_t>(data));
      break;
    case ZCM_FIELD_BYTE:
      v->numerics.emplace_back(name, toDouble<uint8_t>(data));
      break;
    case ZCM_FIELD_FLOAT:
      v->numerics.emplace_back(name, toDouble<float>(data));
      break;
    case ZCM_FIELD_DOUBLE:
      v->numerics.emplace_back(name, toDouble<double>(data));
      break;
    case ZCM_FIELD_BOOLEAN:
      v->numerics.emplace_back(name, toDouble<bool>(data));
      break;
    case ZCM_FIELD_STRING:
      v->strings.emplace_back(name, string((const char*)data));
      break;
    case ZCM_FIELD_USER_TYPE:
      assert(false && "Should not be possble");
  }
};

bool DataLoadZcm::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  string filepath = info->filename.toStdString();

  if (info->plugin_config.hasChildNodes())
  {
    xmlLoadState(info->plugin_config.firstChildElement());
  }
  if (filepath != _all_channels_filepath)
  {
    refreshChannels(filepath);
  }
  if (!launchDialog(filepath))
  {
    return false;
  }

  zcm::TypeDb types(_config_widget->getLibraries().toStdString());
  if (!types.good())
  {
    QMessageBox::warning(nullptr, "Error", "Failed to load zcmtypes");
    return false;
  }

  vector<pair<string, double>> numerics;
  vector<pair<string, string>> strings;
  ProcessUsr usr = { numerics, strings };

  auto processEvent = [&](const zcm::LogEvent* evt) {
    if (_selected_channels.find(evt->channel) == _selected_channels.end())
    {
      return;
    }

    if (evt->datalen == 0)
    {
      auto itr = plot_data.numeric.find(evt->channel);
      if (itr == plot_data.numeric.end())
        itr = plot_data.addNumeric(evt->channel);
      itr->second.pushBack({ (double)evt->timestamp / 1e6, 0 });
      return;
    }

    zcm::Introspection::processEncodedType(evt->channel, evt->data, evt->datalen, "/",
                                           types, processData, &usr);
    for (auto& n : usr.numerics)
    {
      auto itr = plot_data.numeric.find(n.first);
      if (itr == plot_data.numeric.end())
        itr = plot_data.addNumeric(n.first);
      itr->second.pushBack({ (double)evt->timestamp / 1e6, n.second });
    }
    for (auto& s : usr.strings)
    {
      auto itr = plot_data.strings.find(s.first);
      if (itr == plot_data.strings.end())
        itr = plot_data.addStringSeries(s.first);
      itr->second.pushBack({ (double)evt->timestamp / 1e6, s.second });
    }

    usr.numerics.clear();
    usr.strings.clear();
  };

  if (processInputLog(filepath, processEvent) != 0)
    return false;

  return true;
}

static QDomElement serialize(const unordered_set<string>& input, QDomDocument& doc,
                             const string& name)
{
  QDomElement ret = doc.createElement(QString::fromStdString(name));

  for (const auto& item : input)
  {
    QDomElement stringElement = doc.createElement("String");
    QDomText textNode = doc.createTextNode(QString::fromStdString(item));
    stringElement.appendChild(textNode);
    ret.appendChild(stringElement);
  }

  return ret;
}

static unordered_set<string> deserialize(const QDomElement& elt)
{
  unordered_set<string> ret;

  QDomNodeList childNodes = elt.childNodes();
  for (int i = 0; i < childNodes.size(); ++i)
  {
    QDomNode node = childNodes.item(i);
    if (node.isElement())
    {
      QDomElement element = node.toElement();
      if (element.tagName() == "String")
      {
        QString stringValue = element.text();
        ret.insert(stringValue.toStdString());
      }
    }
  }

  return ret;
}

bool DataLoadZcm::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  parent_element.appendChild(serialize(_selected_channels, doc, "selected_channels"));
  auto all_channels = serialize(_all_channels, doc, "all_channels");
  all_channels.setAttribute("filepath", QString::fromStdString(_all_channels_filepath));
  parent_element.appendChild(all_channels);
  return true;
}

bool DataLoadZcm::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement all_channels = parent_element.firstChildElement("all_channels");
  if (!all_channels.isNull())
  {
    _all_channels = deserialize(all_channels);
    if (all_channels.hasAttribute("filepath"))
    {
      _all_channels_filepath = all_channels.attribute("filepath").toStdString();
    }
  }

  QDomElement selected_channels = parent_element.firstChildElement("selected_channels");
  if (!selected_channels.isNull())
  {
    _selected_channels = deserialize(selected_channels);

    QStringList selected_channels;
    for (auto& c : _selected_channels)
      selected_channels.push_back(QString::fromStdString(c));

    QSettings settings;
    settings.setValue("DataLoadZcm.selected_channels", selected_channels);
  }

  return true;
}
