#include "dataload_ulog.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QWidget>
#include <QSettings>
#include <QProgressDialog>
#include <QMainWindow>
#include "selectlistdialog.h"
#include "ulog_parser.h"
#include "ulog_parameters_dialog.h"

DataLoadULog::DataLoadULog() : _main_win(nullptr)
{
  for (QWidget* widget : qApp->topLevelWidgets())
  {
    if (widget->inherits("QMainWindow"))
    {
      _main_win = widget;
      break;
    }
  }
}

const std::vector<const char*>& DataLoadULog::compatibleFileExtensions() const
{
  static std::vector<const char*> extensions = { "ulg" };
  return extensions;
}

bool DataLoadULog::readDataFromFile(FileLoadInfo* fileload_info, PlotDataMapRef& plot_data)
{
  const auto& filename = fileload_info->filename;

  QFile file(filename);

  if (!file.open(QIODevice::ReadOnly))
  {
    throw std::runtime_error("ULog: Failed to open file");
  }
  QByteArray file_array = file.readAll();
  ULogParser::DataStream datastream( file_array.data(), file_array.size() );

  ULogParser parser(datastream);

  const auto& timeseries_map = parser.getTimeseriesMap();

  for (const auto& it : timeseries_map)
  {
    const std::string& sucsctiption_name = it.first;
    const ULogParser::Timeseries& timeseries = it.second;

    for (const auto& data : timeseries.data)
    {
      std::string series_name = sucsctiption_name + data.first;

      auto series = plot_data.addNumeric(series_name);

      for (size_t i = 0; i < data.second.size(); i++)
      {
        double msg_time = static_cast<double>(timeseries.timestamps[i]) * 0.000001;
        PlotData::Point point(msg_time, data.second[i]);
        series->second.pushBack(point);
      }
    }
  }

  ULogParametersDialog* dialog = new ULogParametersDialog(parser, _main_win);
  dialog->setWindowTitle(QString("ULog file %1").arg(filename));
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->restoreSettings();
  dialog->show();

  return true;
}

DataLoadULog::~DataLoadULog()
{
}

bool DataLoadULog::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  return true;
}

bool DataLoadULog::xmlLoadState(const QDomElement&)
{
  return true;
}
