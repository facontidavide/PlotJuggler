#include "dataload_ulog.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QWidget>
#include <QSettings>
#include <QMainWindow>

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
  ULogParser::DataStream datastream(file_array.data(), file_array.size());

  ULogParser parser(datastream);

  const auto& timeseries_map = parser.getTimeseriesMap();
  auto min_msg_time = std::numeric_limits<double>::max();
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
        const uint64_t timestamp = timeseries.timestamps[i].value_or(static_cast<uint64_t>(i));
        double msg_time = static_cast<double>(timestamp) * 0.000001;
        min_msg_time = std::min(min_msg_time, msg_time);
        PlotData::Point point(msg_time, data.second[i]);
        series->second.pushBack(point);
      }
    }
  }

  // store parameters as a timeseries with a single point
  for (const auto& param : parser.getParameters())
  {
    auto series = plot_data.addNumeric("_parameters/" + param.name);
    double value = (param.val_type == ULogParser::FLOAT) ? double(param.value.val_real) :
                                                           double(param.value.val_int);
    series->second.pushBack({ min_msg_time, value });
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
