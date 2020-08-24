#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDebug>
#include <QMessageBox>
#include <QAbstractItemView>
#include <set>
#include "dialog_select_ros_topics.h"
#include "ui_dialog_select_ros_topics.h"

DialogSelectRosTopics::DialogSelectRosTopics(const std::vector<std::pair<QString, QString>>& topic_list,
                                             const Configuration& config, QWidget* parent)
  : QDialog(parent)
  , ui(new Ui::dialogSelectRosTopics)
  , _default_selected_topics(config.selected_topics)
  , _select_all(QKeySequence(Qt::CTRL + Qt::Key_A), this)
  , _deselect_all(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_A), this)
{
  auto flags = this->windowFlags();
  this->setWindowFlags(flags | Qt::WindowStaysOnTopHint);

  ui->setupUi(this);

  ui->checkBoxEnableRules->setChecked(config.use_renaming_rules);
  ui->spinBoxArraySize->setValue(config.max_array_size);
  ui->checkBoxTimestamp->setChecked(config.use_header_stamp);

  if (config.discard_large_arrays)
  {
    ui->radioMaxDiscard->setChecked(true);
  }
  else
  {
    ui->radioMaxClamp->setChecked(true);
  }

  QStringList labels;
  labels.push_back("Topic name");
  labels.push_back("Datatype");

  ui->listRosTopics->setHorizontalHeaderLabels(labels);
  ui->listRosTopics->verticalHeader()->setVisible(false);

  updateTopicList(topic_list);

  if (ui->listRosTopics->rowCount() == 1)
  {
    ui->listRosTopics->selectRow(0);
  }
  ui->listRosTopics->setFocus();

  _select_all.setContext(Qt::WindowShortcut);
  _deselect_all.setContext(Qt::WindowShortcut);

  connect(&_select_all, &QShortcut::activated, ui->listRosTopics, [this]() {
    for (int row = 0; row < ui->listRosTopics->rowCount(); row++)
    {
      if (!ui->listRosTopics->isRowHidden(row) && !ui->listRosTopics->item(row, 0)->isSelected())
      {
        ui->listRosTopics->selectRow(row);
      }
    }
  });

  connect(&_deselect_all, &QShortcut::activated, ui->listRosTopics, &QAbstractItemView::clearSelection);

  on_spinBoxArraySize_valueChanged(ui->spinBoxArraySize->value());

  QSettings settings;
  restoreGeometry(settings.value("DialogSelectRosTopics.geometry").toByteArray());

#ifdef DISABLE_RULE_EDITING
  ui->checkBoxEnableRules->setHidden(true);
  ui->pushButtonEditRules->setHidden(true);
#endif
}

void DialogSelectRosTopics::updateTopicList(std::vector<std::pair<QString, QString>> topic_list)
{
  std::set<QString> newly_added;

  // add if not present
  for (const auto& it : topic_list)
  {
    const QString& topic_name = it.first;
    const QString& type_name = it.second;

    bool found = false;
    for (int r = 0; r < ui->listRosTopics->rowCount(); r++)
    {
      const QTableWidgetItem* item = ui->listRosTopics->item(r, 0);
      if (item->text() == topic_name)
      {
        found = true;
        break;
      }
    }

    if (!found)
    {
      int new_row = ui->listRosTopics->rowCount();
      ui->listRosTopics->setRowCount(new_row + 1);

      // order IS important, don't change it
      ui->listRosTopics->setItem(new_row, 1, new QTableWidgetItem(type_name));
      ui->listRosTopics->setItem(new_row, 0, new QTableWidgetItem(topic_name));
      newly_added.insert(topic_name);
    }
  }

  if (newly_added.size() > 1)
  {
    ui->listRosTopics->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->listRosTopics->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->listRosTopics->sortByColumn(0, Qt::AscendingOrder);
  }

  QModelIndexList selection = ui->listRosTopics->selectionModel()->selectedRows();

  for (int row = 0; row < ui->listRosTopics->rowCount(); row++)
  {
    const QTableWidgetItem* item = ui->listRosTopics->item(row, 0);
    QString topic_name = item->text();

    if (newly_added.count(topic_name) && _default_selected_topics.contains(topic_name))
    {
      bool selected = false;
      for (const auto& selected_item : selection)
      {
        if (selected_item.row() == row)
        {
          selected = true;
          break;
        }
      }
      if (!selected)
      {
        ui->listRosTopics->selectRow(row);
      }
    }
  }
}

DialogSelectRosTopics::~DialogSelectRosTopics()
{
  QSettings settings;
  settings.setValue("DialogSelectRosTopics.geometry", saveGeometry());
  delete ui;
}

DialogSelectRosTopics::Configuration DialogSelectRosTopics::getResult() const
{
  Configuration config;
  config.selected_topics = _topic_list;
  config.max_array_size = ui->spinBoxArraySize->value();
  config.use_header_stamp = ui->checkBoxTimestamp->isChecked();
  config.discard_large_arrays = ui->radioMaxDiscard->isChecked();
  config.use_renaming_rules = ui->checkBoxEnableRules->isChecked();
  return config;
}

void DialogSelectRosTopics::on_buttonBox_accepted()
{
  QModelIndexList selected_indexes = ui->listRosTopics->selectionModel()->selectedIndexes();
  QString selected_topics;

  foreach (QModelIndex index, selected_indexes)
  {
    if (index.column() == 0)
    {
      _topic_list.push_back(index.data(Qt::DisplayRole).toString());
      selected_topics.append(_topic_list.back()).append(" ");
    }
  }
}

void DialogSelectRosTopics::on_listRosTopics_itemSelectionChanged()
{
  QModelIndexList indexes = ui->listRosTopics->selectionModel()->selectedIndexes();

  ui->buttonBox->setEnabled(indexes.size() > 0);
}

void DialogSelectRosTopics::on_checkBoxEnableRules_toggled(bool checked)
{
  ui->pushButtonEditRules->setEnabled(checked);
}

void DialogSelectRosTopics::on_pushButtonEditRules_pressed()
{
#ifndef DISABLE_RULE_EDITING
  RuleEditing* rule_editing = new RuleEditing(this);
  rule_editing->exec();
#endif
}

/*
nonstd::optional<double> FlatContainerContainHeaderStamp(const RosIntrospection::FlatMessage& flat_msg)
{
    for (const auto& it: flat_msg.value)
    {
        if( it.second.getTypeID() != RosIntrospection::TIME)
        {
            continue;
        }
        const RosIntrospection::StringTreeNode* leaf1 = it.first.node_ptr;
        const RosIntrospection::StringTreeNode* leaf2 = leaf1->parent();
        if( leaf2 && leaf2->value() == "header" && leaf1->value() == "stamp")
        {
            return it.second.convert<double>();
        }
    }
    return nonstd::optional<double>();
}
*/

void DialogSelectRosTopics::on_maximumSizeHelp_pressed()
{
  QMessageBox msgBox;
  msgBox.setWindowTitle("Help");
  msgBox.setText("Maximum Size of Arrays:\n\n"
                 "If the size of an Arrays is larger than this maximum value, the entire array is skipped.\n\n"
                 "This parameter is used to prevent the user from loading HUGE arrays, "
                 "such as images, pointclouds, maps, etc.\n"
                 "The term 'array' refers to the array in a message field,\n\n"
                 " See http://wiki.ros.org/msg.\n\n"
                 "This is NOT about the duration of a time series!\n\n"
                 "MOTIVATION: pretend that a user tries to load a RGB image, which probably contains "
                 "a few millions pixels.\n"
                 "Plotjuggler would naively create a single time series for each pixel of the image! "
                 "That makes no sense, of course, and it would probably freeze your system.\n");
  msgBox.exec();
}

void DialogSelectRosTopics::on_lineEditFilter_textChanged(const QString& search_string)
{
  int visible_count = 0;
  bool updated = false;

  QStringList spaced_items = search_string.split(' ');

  for (int row = 0; row < ui->listRosTopics->rowCount(); row++)
  {
    auto item = ui->listRosTopics->item(row, 0);
    QString name = item->text();
    int pos = 0;
    bool toHide = false;

    for (const auto& item : spaced_items)
    {
      if (!name.contains(item))
      {
        toHide = true;
        break;
      }
    }
    ui->listRosTopics->setRowHidden(row, toHide);
  }
}

void DialogSelectRosTopics::on_spinBoxArraySize_valueChanged(int value)
{
}
