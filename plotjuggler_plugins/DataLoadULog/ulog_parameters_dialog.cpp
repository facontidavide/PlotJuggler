#include "ulog_parameters_dialog.h"
#include "ui_ulog_parameters_dialog.h"

#include <QTableWidget>
#include <QSettings>
#include <QHeaderView>

ULogParametersDialog::ULogParametersDialog(const ULogParser& parser, QWidget* parent)
  : QDialog(parent), ui(new Ui::ULogParametersDialog)
{
  ui->setupUi(this);
  QTableWidget* table_info = ui->tableWidgetInfo;
  QTableWidget* table_params = ui->tableWidgetParams;
  QTableWidget* table_logs = ui->tableWidgetLogs;

  table_info->setRowCount(parser.getInfo().size());
  int row = 0;
  for (const auto& it : parser.getInfo())
  {
    table_info->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(it.first)));
    table_info->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(it.second)));
    row++;
  }
  table_info->sortItems(0);

  table_params->setRowCount(parser.getParameters().size());
  row = 0;
  for (const auto& param : parser.getParameters())
  {
    table_params->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(param.name)));
    if (param.val_type == ULogParser::FLOAT)
    {
      table_params->setItem(row, 1, new QTableWidgetItem(QString::number(param.value.val_real)));
    }
    else
    {
      table_params->setItem(row, 1, new QTableWidgetItem(QString::number(param.value.val_int)));
    }
    row++;
  }
  table_params->sortItems(0);

  table_logs->setRowCount(parser.getLogs().size());
  row = 0;
  for (const auto& log_msg : parser.getLogs())
  {
    QString time = QString::number(0.001 * double(log_msg.timestamp / 1000), 'f', 2);
    table_logs->setItem(row, 0, new QTableWidgetItem(time));

    switch (log_msg.level)
    {
      case '0':
        table_logs->setItem(row, 1, new QTableWidgetItem("EMERGENCY"));
        break;
      case '1':
        table_logs->setItem(row, 1, new QTableWidgetItem("ALERT"));
        break;
      case '2':
        table_logs->setItem(row, 1, new QTableWidgetItem("CRITICAL"));
        break;
      case '3':
        table_logs->setItem(row, 1, new QTableWidgetItem("ERROR"));
        break;
      case '4':
        table_logs->setItem(row, 1, new QTableWidgetItem("WARNING"));
        break;
      case '5':
        table_logs->setItem(row, 1, new QTableWidgetItem("NOTICE"));
        break;
      case '6':
        table_logs->setItem(row, 1, new QTableWidgetItem("INFO"));
        break;
      case '7':
        table_logs->setItem(row, 1, new QTableWidgetItem("DEBUG"));
        break;
      default:
        table_logs->setItem(row, 1, new QTableWidgetItem(QString::number(log_msg.level)));
    }
    table_logs->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(log_msg.msg)));
    row++;
  }
}

void ULogParametersDialog::restoreSettings()
{
  QTableWidget* table_info = ui->tableWidgetInfo;
  QTableWidget* table_params = ui->tableWidgetParams;

  QSettings settings;
  restoreGeometry(settings.value("ULogParametersDialog/geometry").toByteArray());
  table_info->horizontalHeader()->restoreState(settings.value("ULogParametersDialog/info/state").toByteArray());
  table_params->horizontalHeader()->restoreState(settings.value("ULogParametersDialog/params/state").toByteArray());

  table_info->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  table_info->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);

  table_params->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  table_params->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
}

ULogParametersDialog::~ULogParametersDialog()
{
  QTableWidget* table_info = ui->tableWidgetInfo;
  QTableWidget* table_params = ui->tableWidgetParams;

  QSettings settings;
  settings.setValue("ULogParametersDialog/geometry", this->saveGeometry());
  settings.setValue("ULogParametersDialog/info/state", table_info->horizontalHeader()->saveState());
  settings.setValue("ULogParametersDialog/params/state", table_params->horizontalHeader()->saveState());

  delete ui;
}
