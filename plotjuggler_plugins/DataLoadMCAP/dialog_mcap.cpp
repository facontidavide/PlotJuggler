#include "dialog_mcap.h"
#include "ui_dialog_mcap.h"

#include <QSettings>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QElapsedTimer>

#include <mcap/reader.hpp>

const QString DialogMCAP::prefix = "DialogLoadMCAP::";

DialogMCAP::DialogMCAP(const std::unordered_map<int, mcap::ChannelPtr>& channels,
                       const std::unordered_map<int, mcap::SchemaPtr>& schemas,
                       std::optional<mcap::LoadParams> default_parameters,
                       QWidget* parent)
  : QDialog(parent), ui(new Ui::dialog_mcap)
{
  ui->setupUi(this);

  ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->tableWidget->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  ui->tableWidget->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);

  ui->tableWidget->setRowCount(channels.size());

  QSettings settings;
  restoreGeometry(settings.value(prefix + "geometry").toByteArray());

  mcap::LoadParams params;
  if (!default_parameters)
  {
    params.selected_topics = settings.value(prefix + "selected").toStringList();
    params.clamp_large_arrays = settings.value(prefix + "clamp", true).toBool();
    params.max_array_size = settings.value(prefix + "max_array", 500).toInt();
    params.use_timestamp = settings.value(prefix + "use_timestamp", false).toBool();
    params.use_mcap_log_time =
        settings.value(prefix + "use_mcap_log_time", false).toBool();
  }
  else
  {
    params = *default_parameters;
  }

  if (params.clamp_large_arrays)
  {
    ui->radioClamp->setChecked(true);
  }
  else
  {
    ui->radioSkip->setChecked(true);
  }
  ui->spinBox->setValue(params.max_array_size);
  ui->checkBoxUseTimestamp->setChecked(params.use_timestamp);
  if (params.use_mcap_log_time)
  {
    ui->radioLogTime->setChecked(true);
  }
  else
  {
    ui->radioPubTime->setChecked(true);
  }

  int row = 0;
  for (const auto& [id, channel] : channels)
  {
    auto topic = QString::fromStdString(channel->topic);
    auto const& schema = schemas.at(channel->schemaId);

    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(topic));
    ui->tableWidget->setItem(row, 1,
                             new QTableWidgetItem(QString::fromStdString(schema->name)));
    ui->tableWidget->setItem(
        row, 2, new QTableWidgetItem(QString::fromStdString(schema->encoding)));

    if (params.selected_topics.contains(topic))
    {
      ui->tableWidget->selectRow(row);
    }
    row++;
  }
  ui->tableWidget->sortByColumn(0, Qt::SortOrder::DescendingOrder);
}

DialogMCAP::~DialogMCAP()
{
  delete ui;
}

mcap::LoadParams DialogMCAP::getParams() const
{
  mcap::LoadParams params;
  params.max_array_size = ui->spinBox->value();
  params.clamp_large_arrays = ui->radioClamp->isChecked();
  params.use_timestamp = ui->checkBoxUseTimestamp->isChecked();
  params.use_mcap_log_time = ui->radioLogTime->isChecked();

  QItemSelectionModel* select = ui->tableWidget->selectionModel();
  QStringList selected_topics;
  for (QModelIndex index : select->selectedRows())
  {
    params.selected_topics.push_back(ui->tableWidget->item(index.row(), 0)->text());
  }
  return params;
}

void DialogMCAP::on_tableWidget_itemSelectionChanged()
{
  bool enabled = !ui->tableWidget->selectionModel()->selectedRows().empty();
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enabled);
}

void DialogMCAP::accept()
{
  QSettings settings;
  settings.setValue(prefix + "geometry", saveGeometry());

  bool clamp_checked = ui->radioClamp->isChecked();
  int max_array = ui->spinBox->value();
  bool use_timestamp = ui->checkBoxUseTimestamp->isChecked();
  bool use_mcap_log_time = ui->radioLogTime->isChecked();

  settings.setValue(prefix + "clamp", clamp_checked);
  settings.setValue(prefix + "max_array", max_array);
  settings.setValue(prefix + "use_timestamp", use_timestamp);
  settings.setValue(prefix + "use_mcap_log_time", use_mcap_log_time);

  QItemSelectionModel* select = ui->tableWidget->selectionModel();
  QStringList selected_topics;
  for (QModelIndex index : select->selectedRows())
  {
    selected_topics.push_back(ui->tableWidget->item(index.row(), 0)->text());
  }
  settings.setValue(prefix + "selected", selected_topics);

  QDialog::accept();
}
