#include "dialog_blf.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>

#include <unordered_set>

#include "ui_dialog_blf.h"

namespace PJ::BLF
{

DialogBLF::DialogBLF(QWidget* parent) : QDialog(parent), ui_(new Ui::DialogBLF)
{
  ui_->setupUi(this);

  ui_->tableChannelMap->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  ui_->tableChannelMap->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

  connect(ui_->btnAddDbc, &QPushButton::clicked, this, &DialogBLF::OnAddDbc);
  connect(ui_->btnRemoveDbc, &QPushButton::clicked, this, &DialogBLF::OnRemoveDbc);
  connect(ui_->btnAddMapRow, &QPushButton::clicked, this, &DialogBLF::OnAddMapRow);
  connect(ui_->btnRemoveMapRow, &QPushButton::clicked, this, &DialogBLF::OnRemoveMapRow);
  connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &DialogBLF::accept);
  connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &DialogBLF::reject);
}

DialogBLF::~DialogBLF()
{
  delete ui_;
}

void DialogBLF::SetConfig(const BlfPluginConfig& config)
{
  ui_->checkEmitRaw->setChecked(config.emit_raw);
  ui_->checkEmitDecoded->setChecked(config.emit_decoded);
  ui_->checkUseSourceTs->setChecked(config.use_source_timestamp);

  ui_->listDbcFiles->clear();
  for (const auto& dbc_file : config.dbc_files)
  {
    ui_->listDbcFiles->addItem(QString::fromStdString(dbc_file));
  }

  ui_->tableChannelMap->setRowCount(0);
  for (const auto& [channel, dbc_file] : config.channel_to_dbc)
  {
    AppendMapRow(channel, QString::fromStdString(dbc_file));
  }
}

BlfPluginConfig DialogBLF::GetConfig() const
{
  BlfPluginConfig config;
  config.emit_raw = ui_->checkEmitRaw->isChecked();
  config.emit_decoded = ui_->checkEmitDecoded->isChecked();
  config.use_source_timestamp = ui_->checkUseSourceTs->isChecked();

  config.dbc_files.reserve(static_cast<std::size_t>(ui_->listDbcFiles->count()));
  for (int i = 0; i < ui_->listDbcFiles->count(); ++i)
  {
    const QListWidgetItem* item = ui_->listDbcFiles->item(i);
    if (item)
    {
      const QString path = item->text().trimmed();
      if (!path.isEmpty())
      {
        config.dbc_files.push_back(path.toStdString());
      }
    }
  }

  const int rows = ui_->tableChannelMap->rowCount();
  for (int row = 0; row < rows; ++row)
  {
    const QTableWidgetItem* channel_item = ui_->tableChannelMap->item(row, 0);
    const QTableWidgetItem* dbc_item = ui_->tableChannelMap->item(row, 1);

    if (!channel_item || !dbc_item)
    {
      continue;
    }

    bool ok = false;
    const uint32_t channel = channel_item->text().trimmed().toUInt(&ok);
    const QString dbc_path = dbc_item->text().trimmed();
    if (!ok || dbc_path.isEmpty())
    {
      continue;
    }
    config.channel_to_dbc[channel] = dbc_path.toStdString();
  }

  return config;
}

void DialogBLF::OnAddDbc()
{
  const QStringList paths = QFileDialog::getOpenFileNames(
      this, tr("Select DBC Files"), QString(), tr("DBC Files (*.dbc);;All Files (*)"));
  for (const QString& path : paths)
  {
    if (path.trimmed().isEmpty())
    {
      continue;
    }

    QList<QListWidgetItem*> found = ui_->listDbcFiles->findItems(path, Qt::MatchExactly);
    if (found.isEmpty())
    {
      ui_->listDbcFiles->addItem(path);
    }
  }
}

void DialogBLF::OnRemoveDbc()
{
  qDeleteAll(ui_->listDbcFiles->selectedItems());
}

void DialogBLF::OnAddMapRow()
{
  const QString default_dbc =
      (ui_->listDbcFiles->count() > 0) ? ui_->listDbcFiles->item(0)->text() : QString();
  AppendMapRow(0U, default_dbc);
}

void DialogBLF::OnRemoveMapRow()
{
  const auto ranges = ui_->tableChannelMap->selectedRanges();
  if (ranges.isEmpty())
  {
    return;
  }

  for (int i = ranges.size() - 1; i >= 0; --i)
  {
    const int start = ranges[i].topRow();
    const int end = ranges[i].bottomRow();
    for (int row = end; row >= start; --row)
    {
      ui_->tableChannelMap->removeRow(row);
    }
  }
}

void DialogBLF::accept()
{
  if (!ui_->checkEmitRaw->isChecked() && !ui_->checkEmitDecoded->isChecked())
  {
    QMessageBox::warning(this, tr("Invalid Configuration"),
                         tr("At least one output mode must be enabled."));
    return;
  }

  std::unordered_set<uint32_t> seen_channels;
  const int rows = ui_->tableChannelMap->rowCount();
  for (int row = 0; row < rows; ++row)
  {
    const QTableWidgetItem* channel_item = ui_->tableChannelMap->item(row, 0);
    const QTableWidgetItem* dbc_item = ui_->tableChannelMap->item(row, 1);

    if (!channel_item || !dbc_item)
    {
      QMessageBox::warning(this, tr("Invalid Mapping"),
                           tr("Mapping row %1 has empty cells.").arg(row + 1));
      return;
    }

    bool ok = false;
    const uint32_t channel = channel_item->text().trimmed().toUInt(&ok);
    if (!ok)
    {
      QMessageBox::warning(this, tr("Invalid Mapping"),
                           tr("Mapping row %1 has an invalid channel value.").arg(row + 1));
      return;
    }
    if (!seen_channels.insert(channel).second)
    {
      QMessageBox::warning(
          this, tr("Invalid Mapping"),
          tr("Mapping row %1 repeats channel %2. Each channel must be unique.")
              .arg(row + 1)
              .arg(channel));
      return;
    }

    if (dbc_item->text().trimmed().isEmpty())
    {
      QMessageBox::warning(this, tr("Invalid Mapping"),
                           tr("Mapping row %1 has an empty DBC path.").arg(row + 1));
      return;
    }
  }

  QDialog::accept();
}

void DialogBLF::AppendMapRow(uint32_t channel, const QString& dbc_path)
{
  const int row = ui_->tableChannelMap->rowCount();
  ui_->tableChannelMap->insertRow(row);

  auto* channel_item = new QTableWidgetItem(QString::number(channel));
  auto* dbc_item = new QTableWidgetItem(dbc_path);

  ui_->tableChannelMap->setItem(row, 0, channel_item);
  ui_->tableChannelMap->setItem(row, 1, dbc_item);
}

}  // namespace PJ::BLF
