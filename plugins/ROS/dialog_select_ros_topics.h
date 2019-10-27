#ifndef DIALOG_SELECT_ROS_TOPICS_H
#define DIALOG_SELECT_ROS_TOPICS_H

#include <QDialog>
#include <QString>
#include <QFile>
#include <QStringList>
#include <QCheckBox>
#include <QShortcut>
#include "PlotJuggler/optional.hpp"
#include <ros_type_introspection/ros_introspection.hpp>

namespace Ui {
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
	QString prefix;
	QString rm_prefixes;

	std::vector<std::string> getRemovePrefixes()
	{
       	   std::vector<std::string> std_rm_prefixes;
           QStringList rmlist = rm_prefixes.split(';', QString::SkipEmptyParts);
           for (const QString & rm : rmlist)
              std_rm_prefixes.emplace_back(rm.toStdString());
           return std_rm_prefixes;  
	}
    };

    explicit DialogSelectRosTopics(const std::vector<std::pair<QString,QString>>& topic_list,
                                   const Configuration& default_info,
                                   QWidget *parent = nullptr);

    ~DialogSelectRosTopics() override;

    Configuration getResult() const;

public slots:

    void updateTopicList(std::vector<std::pair<QString,QString>> topic_list);

private slots:

    void on_buttonBox_accepted();

    void on_listRosTopics_itemSelectionChanged();

    void on_checkBoxEnableRules_toggled(bool checked);

    void on_pushButtonEditRules_pressed();

    void on_maximumSizeHelp_pressed();

    void on_lineEditFilter_textChanged(const QString &search_string);

    void on_spinBoxArraySize_valueChanged(int value);

    void on_checkBoxPrefix_toggled(bool checked);

private:

    QStringList _topic_list;
    QStringList _default_selected_topics;

    QShortcut _select_all;
    QShortcut _deselect_all;

    Ui::dialogSelectRosTopics *ui;

};

nonstd::optional<double>FlatContainerContainHeaderStamp(const RosIntrospection::FlatMessage& flat_msg);



#endif // DIALOG_SELECT_ROS_TOPICS_H
