#include "dialog_mcap.h"
#include "ui_dialog_mcap.h"

#include <QSettings>
#include <QDialogButtonBox>
#include <QPushButton>

const QString DialogMCAP::prefix = "DialogLoadMCAP::";


DialogMCAP::DialogMCAP(const std::unordered_map<int, mcap::ChannelPtr> &channels,
                       const std::unordered_map<int, mcap::SchemaPtr> &schemas,
                       QWidget *parent) :
  QDialog(parent),
  ui(new Ui::dialog_mcap)
{
  ui->setupUi(this);

  ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

  ui->tableWidget->setRowCount(channels.size());

  QSettings settings;

  restoreGeometry(settings.value(prefix + "geometry").toByteArray());
  auto selected = settings.value(prefix + "selected").toStringList();
  bool clamp_checked = settings.value(prefix + "clamp", true).toBool();
  int max_array = settings.value(prefix + "max_array", 500).toInt();

  if( clamp_checked )
  {
    ui->radioClamp->setChecked(true);
  }
  else{
    ui->radioSkip->setChecked(true);
  }
  ui->spinBox->setValue( max_array );

  int row = 0;
  for(const auto& [id, channel]: channels )
  {
    auto topic = QString::fromStdString(channel->topic);
    auto encoding = channel->messageEncoding;
    auto schema = schemas.at( channel->schemaId )->name;

    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(topic) );
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(schema)) );
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(encoding)) );

    if( selected.contains(topic) )
    {
      ui->tableWidget->selectRow(row);
    }
    row++;
  }
  ui->tableWidget->sortByColumn(0);


}

DialogMCAP::~DialogMCAP()
{
  delete ui;
}

DialogMCAP::Params DialogMCAP::getParams() const
{
  Params params;
  params.max_array_size = ui->spinBox->value();
  params.clamp_large_arrays = ui->radioClamp->isChecked();

  QItemSelectionModel *select = ui->tableWidget->selectionModel();
  QStringList selected_topics;
  for(QModelIndex index: select->selectedRows())
  {
    params.selected_topics.push_back( ui->tableWidget->item(index.row(), 0)->text() );
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

  settings.setValue(prefix + "clamp", clamp_checked);
  settings.setValue(prefix + "max_array", max_array);

  QItemSelectionModel *select = ui->tableWidget->selectionModel();
  QStringList selected_topics;
  for(QModelIndex index: select->selectedRows())
  {
    selected_topics.push_back( ui->tableWidget->item(index.row(), 0)->text() );
  }
  settings.setValue(prefix + "selected", selected_topics);

  QDialog::accept();
}
