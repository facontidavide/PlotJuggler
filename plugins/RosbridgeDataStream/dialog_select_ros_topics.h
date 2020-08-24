#ifndef DIALOG_SELECT_ROS_TOPICS_H
#define DIALOG_SELECT_ROS_TOPICS_H

#include <QDialog>
#include <QString>
#include <QFile>
#include <QStringList>
#include <QCheckBox>
#include <QShortcut>
#include "PlotJuggler/optional.hpp"

#ifndef DISABLE_RULE_EDITING
#include "rule_editing.h"
#endif

namespace Ui
{
class dialogSelectRosTopics;
}

class DialogSelectRosTopics : public QDialog
{
  Q_OBJECT

public:
  struct Configuration
  {
    QStringList selected_topics;
    size_t max_array_size;
    bool use_header_stamp;
    bool use_renaming_rules;
    bool discard_large_arrays;
  };

  explicit DialogSelectRosTopics(const std::vector<std::pair<QString, QString>>& topic_list,
                                 const Configuration& default_info, QWidget* parent = nullptr);

  ~DialogSelectRosTopics() override;

  Configuration getResult() const;

public slots:

  void updateTopicList(std::vector<std::pair<QString, QString>> topic_list);

private slots:

  void on_buttonBox_accepted();

  void on_listRosTopics_itemSelectionChanged();

  void on_checkBoxEnableRules_toggled(bool checked);

  void on_pushButtonEditRules_pressed();

  void on_maximumSizeHelp_pressed();

  void on_lineEditFilter_textChanged(const QString& search_string);

  void on_spinBoxArraySize_valueChanged(int value);

private:
  QStringList _topic_list;
  QStringList _default_selected_topics;

  QShortcut _select_all;
  QShortcut _deselect_all;

  Ui::dialogSelectRosTopics* ui;
};

#endif  // DIALOG_SELECT_ROS_TOPICS_H
