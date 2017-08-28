#ifndef DIALOG_SELECT_ROS_TOPICS_H
#define DIALOG_SELECT_ROS_TOPICS_H

#include <QDialog>
#include <QString>
#include <QFile>
#include <QStringList>
#include <QCheckBox>
#include <PlotJuggler/optional.hpp>
#include <ros_type_introspection/ros_introspection.hpp>
#include <ros_type_introspection/string.hpp>

namespace Ui {
class dialogSelectRosTopics;
}

class DialogSelectRosTopics : public QDialog
{
    Q_OBJECT

public:

    explicit DialogSelectRosTopics(const std::vector<std::pair<QString,QString>>& topic_list,
                                   QStringList default_selected_topics,
                                   QWidget *parent = 0);

    ~DialogSelectRosTopics();

    QStringList getSelectedItems();

    const QCheckBox* checkBoxUseHeaderStamp();

    const QCheckBox *checkBoxUseRenamingRules();

public slots:

    void updateTopicList(std::vector<std::pair<QString,QString>> topic_list);

private slots:

    void on_buttonBox_accepted();

    void on_listRosTopics_itemSelectionChanged();

    void on_checkBoxEnableRules_toggled(bool checked);

    void on_pushButtonEditRules_pressed();

private:

    void closeEvent(QCloseEvent *event) override;

    QStringList _topic_list;

    Ui::dialogSelectRosTopics *ui;

};

nonstd::optional<double> FlatContainedContainHeaderStamp(const RosIntrospection::RenamedValues& flat_container);

#endif // DIALOG_SELECT_ROS_TOPICS_H
