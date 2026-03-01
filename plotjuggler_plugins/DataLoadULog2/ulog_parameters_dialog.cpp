#include "ulog_parameters_dialog.h"
#include "ui_ulog_parameters_dialog.h"

#include <ulog_cpp/data_container.hpp>

#include <QHeaderView>
#include <QSettings>
#include <QTableWidget>

using BasicType = ulog_cpp::Field::BasicType;

namespace
{

QString paramValueToString(const ulog_cpp::MessageInfo& param)
{
  auto bt = param.field().type().type;
  if (bt == BasicType::FLOAT)
  {
    return QString::number(param.value().as<float>());
  }
  return QString::number(param.value().as<int32_t>());
}

QString paramDefaultValueToString(const ulog_cpp::ParameterDefault& param)
{
  auto bt = param.field().type().type;
  if (bt == BasicType::FLOAT)
  {
    return QString::number(param.value().as<float>());
  }
  return QString::number(param.value().as<int32_t>());
}

QString infoValueToString(const std::string& name, const ulog_cpp::MessageInfo& info)
{
  auto bt = info.field().type().type;

  if (info.field().arrayLength() > 0 && bt == BasicType::CHAR)
  {
    return QString::fromStdString(info.value().as<std::string>());
  }

  switch (bt)
  {
    case BasicType::BOOL:
      return QString::number(info.value().as<int>());
    case BasicType::UINT8:
      return QString::number(info.value().as<uint8_t>());
    case BasicType::INT8:
      return QString::number(info.value().as<int8_t>());
    case BasicType::UINT16:
      return QString::number(info.value().as<uint16_t>());
    case BasicType::INT16:
      return QString::number(info.value().as<int16_t>());
    case BasicType::UINT32: {
      uint32_t v = info.value().as<uint32_t>();
      if (name.rfind("ver_", 0) == 0 && name.size() > 8 &&
          name.compare(name.size() - 8, 8, "_release") == 0)
      {
        return QString("0x%1").arg(v, 8, 16, QChar('0'));
      }
      return QString::number(v);
    }
    case BasicType::INT32:
      return QString::number(info.value().as<int32_t>());
    case BasicType::FLOAT:
      return QString::number(info.value().as<float>());
    case BasicType::DOUBLE:
      return QString::number(info.value().as<double>());
    case BasicType::UINT64:
      return QString::number(info.value().as<uint64_t>());
    case BasicType::INT64:
      return QString::number(info.value().as<int64_t>());
    default:
      return {};
  }
}

}  // namespace

ULogParametersDialog::ULogParametersDialog(const ulog_cpp::DataContainer& container,
                                           QWidget* parent)
  : QDialog(parent), ui(new Ui::ULogParametersDialog)
{
  ui->setupUi(this);

  QTableWidget* table_info = ui->tableWidgetInfo;
  QTableWidget* table_params = ui->tableWidgetParams;
  QTableWidget* table_logs = ui->tableWidgetLogs;

  // --- Info tab ---
  const auto& info_map = container.messageInfo();
  const auto& info_multi = container.messageInfoMulti();

  int info_row_count = static_cast<int>(info_map.size());
  for (const auto& [key, entries] : info_multi)
  {
    for (size_t i = 0; i < entries.size(); i++)
    {
      info_row_count += static_cast<int>(entries[i].size());
    }
  }

  table_info->setRowCount(info_row_count);
  int row = 0;

  for (const auto& [name, info] : info_map)
  {
    table_info->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(name)));
    table_info->setItem(row, 1, new QTableWidgetItem(infoValueToString(name, info)));
    row++;
  }

  for (const auto& [name, entries] : info_multi)
  {
    for (size_t i = 0; i < entries.size(); i++)
    {
      for (size_t j = 0; j < entries[i].size(); j++)
      {
        QString display_name = QString::fromStdString(name) + QString("[%1]").arg(i);
        table_info->setItem(row, 0, new QTableWidgetItem(display_name));
        table_info->setItem(row, 1, new QTableWidgetItem(infoValueToString(name, entries[i][j])));
        row++;
      }
    }
  }
  table_info->setRowCount(row);
  table_info->sortItems(0);

  // --- Parameters tab (3 columns: Name, Value, Default) ---
  // Build final parameter map: initial + changed overrides
  std::map<std::string, const ulog_cpp::MessageInfo*> final_params;
  for (const auto& [name, param] : container.initialParameters())
  {
    final_params[name] = &param;
  }
  for (const auto& param : container.changedParameters())
  {
    final_params[param.field().name()] = &param;
  }

  const auto& defaults = container.defaultParameters();

  table_params->setRowCount(static_cast<int>(final_params.size()));
  row = 0;
  for (const auto& [name, param] : final_params)
  {
    table_params->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(name)));
    table_params->setItem(row, 1, new QTableWidgetItem(paramValueToString(*param)));

    auto def_it = defaults.find(name);
    if (def_it != defaults.end())
    {
      table_params->setItem(row, 2,
                            new QTableWidgetItem(paramDefaultValueToString(def_it->second)));
    }
    else
    {
      table_params->setItem(row, 2, new QTableWidgetItem("-"));
    }
    row++;
  }
  table_params->sortItems(0);

  // --- Logs tab (4 columns: Timestamp, Level, Tag, Message) ---
  const auto& logs = container.logging();
  table_logs->setRowCount(static_cast<int>(logs.size()));
  row = 0;
  for (const auto& log_msg : logs)
  {
    double t_sec = static_cast<double>(log_msg.timestamp()) * 0.000001;
    table_logs->setItem(row, 0, new QTableWidgetItem(QString::number(t_sec, 'f', 2)));
    table_logs->setItem(row, 1,
                        new QTableWidgetItem(QString::fromStdString(log_msg.logLevelStr())));

    if (log_msg.hasTag())
    {
      table_logs->setItem(row, 2, new QTableWidgetItem(QString::number(log_msg.tag())));
    }
    else
    {
      table_logs->setItem(row, 2, new QTableWidgetItem("-"));
    }

    table_logs->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(log_msg.message())));
    row++;
  }

  // --- Diagnostics tab ---
  QStringList diag_lines;

  // File header
  const auto& header = container.fileHeader().header();
  uint64_t file_ts = header.timestamp;
  diag_lines << QString("File timestamp: %1 us").arg(file_ts);
  diag_lines << "";

  // Dropouts
  const auto& dropouts = container.dropouts();
  if (dropouts.empty())
  {
    diag_lines << "Dropouts: none";
  }
  else
  {
    uint64_t total_ms = 0;
    for (const auto& d : dropouts)
    {
      total_ms += d.durationMs();
    }
    diag_lines << QString("Dropouts: %1 (total %2 ms)").arg(dropouts.size()).arg(total_ms);
  }
  diag_lines << "";

  // Parsing errors
  const auto& errors = container.parsingErrors();
  if (errors.empty())
  {
    diag_lines << "Parsing warnings: none";
  }
  else
  {
    diag_lines << QString("Parsing warnings: %1").arg(errors.size());
    for (const auto& err : errors)
    {
      diag_lines << QString("  - %1").arg(QString::fromStdString(err));
    }
  }

  ui->textEditDiagnostics->setPlainText(diag_lines.join("\n"));
}

void ULogParametersDialog::restoreSettings()
{
  QTableWidget* table_info = ui->tableWidgetInfo;
  QTableWidget* table_params = ui->tableWidgetParams;

  QSettings settings;
  restoreGeometry(settings.value("ULogParametersDialog2/geometry").toByteArray());
  table_info->horizontalHeader()->restoreState(
      settings.value("ULogParametersDialog2/info/state").toByteArray());
  table_params->horizontalHeader()->restoreState(
      settings.value("ULogParametersDialog2/params/state").toByteArray());

  table_info->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  table_info->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);

  table_params->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  table_params->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
  table_params->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
}

ULogParametersDialog::~ULogParametersDialog()
{
  QTableWidget* table_info = ui->tableWidgetInfo;
  QTableWidget* table_params = ui->tableWidgetParams;

  QSettings settings;
  settings.setValue("ULogParametersDialog2/geometry", this->saveGeometry());
  settings.setValue("ULogParametersDialog2/info/state",
                    table_info->horizontalHeader()->saveState());
  settings.setValue("ULogParametersDialog2/params/state",
                    table_params->horizontalHeader()->saveState());

  delete ui;
}
