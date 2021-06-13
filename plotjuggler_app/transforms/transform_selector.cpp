#include "transform_selector.h"
#include "ui_transform_selector.h"
#include <QSettings>
#include <QDomDocument>
#include <QTableWidget>
#include <QComboBox>
#include <QDebug>

TransformSelector::TransformSelector(QStringList builtin_transform, QStringList available_transforms,
                                     QString* default_tansform, std::map<std::string, QString>* curve_transforms,
                                     QWidget* parent)
  : QDialog(parent), ui(new Ui::transform_selector), _curves_trans(curve_transforms), _default_trans(default_tansform)
{
  ui->setupUi(this);

  QStringList transforms = builtin_transform + available_transforms;

  ui->comboDefault->insertItems(0, transforms);
  ui->comboDefault->insertSeparator(builtin_transform.size());
  if (transforms.contains(*default_tansform))
  {
    ui->comboDefault->setCurrentText(*default_tansform);
  }

  ui->tableWidget->setRowCount(int(curve_transforms->size()));
  ui->tableWidget->setColumnCount(2);

  int row = 0;
  for (const auto& it : *curve_transforms)
  {
    const auto& trans = it.second;
    auto item_name = new QTableWidgetItem(QString::fromStdString(it.first));
    auto item_combo = new QComboBox();
    item_combo->insertItems(0, transforms);
    item_combo->insertSeparator(3);
    ui->tableWidget->setItem(row, 0, item_name);
    ui->tableWidget->setCellWidget(row, 1, item_combo);
    if (transforms.contains(trans))
    {
      item_combo->setCurrentText(trans);
    }
    row++;
  }
  QHeaderView* header = ui->tableWidget->horizontalHeader();
  header->setSectionResizeMode(QHeaderView::Stretch);
}

TransformSelector::~TransformSelector()
{
  delete ui;
}

void TransformSelector::on_buttonApplyDefault_clicked()
{
  int default_index = ui->comboDefault->currentIndex();
  for (int row = 0; row < ui->tableWidget->rowCount(); row++)
  {
    auto combo = static_cast<QComboBox*>(ui->tableWidget->cellWidget(row, 1));
    combo->setCurrentIndex(default_index);
  }
}

void TransformSelector::on_buttonResetAll_clicked()
{
  for (int row = 0; row < ui->tableWidget->rowCount(); row++)
  {
    auto combo = static_cast<QComboBox*>(ui->tableWidget->cellWidget(row, 1));
    combo->setCurrentIndex(0);
  }
}

void TransformSelector::on_transform_selector_accepted()
{
  for (int row = 0; row < ui->tableWidget->rowCount(); row++)
  {
    const auto& name = ui->tableWidget->item(row, 0)->text();
    auto combo = static_cast<QComboBox*>(ui->tableWidget->cellWidget(row, 1));
    (*_curves_trans)[name.toStdString()] = combo->currentText();
  }
  *_default_trans = ui->comboDefault->currentText();
}
