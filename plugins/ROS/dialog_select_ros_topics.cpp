#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QSettings>
#include <QHeaderView>

#include "dialog_select_ros_topics.h"
#include "rule_editing.h"
#include "ui_dialog_select_ros_topics.h"


DialogSelectRosTopics::DialogSelectRosTopics(const std::vector<std::pair<QString, QString>>& topic_list,
                                             QStringList default_selected_topics,
                                             QWidget *parent) :
    QDialog(parent),
    ui(new Ui::dialogSelectRosTopics),
    _default_selected_topics( default_selected_topics)
{

    auto flags = this->windowFlags();
    this->setWindowFlags( flags | Qt::WindowStaysOnTopHint);

    ui->setupUi(this);

    QSettings settings( "IcarusTechnology", "PlotJuggler");
    ui->checkBoxEnableRules->setChecked(     settings.value("DialogSelectRosTopics.enableRules", true ).toBool());
    ui->checkBoxUseHeaderStamp->setChecked(  settings.value("DialogSelectRosTopics.useHeaderStamp", true ).toBool());
    restoreGeometry(settings.value("DialogSelectRosTopics.geometry").toByteArray());

    if( _default_selected_topics.isEmpty())
    {
        QString default_topics = settings.value("DialogSelectRosTopics.selectedItems", "" ).toString();
        _default_selected_topics = default_topics.split(' ', QString::SkipEmptyParts);
    }

    QStringList labels;
    labels.push_back("Topic name");
    labels.push_back("Datatype");

    ui->listRosTopics->setHorizontalHeaderLabels(labels);
    ui->listRosTopics->verticalHeader()->setVisible(false);

    updateTopicList(topic_list);
}

void DialogSelectRosTopics::updateTopicList(std::vector<std::pair<QString, QString> > topic_list)
{
    // add if not present
    for (int row=0; row< topic_list.size(); row++)
    {
        const QString& topic_name = topic_list[row].first ;
        const QString& type_name  = topic_list[row].second ;

        QList<QTableWidgetItem *> same_name_items = ui->listRosTopics->findItems(topic_name, Qt::MatchExactly);
        bool found = false;
        for (QTableWidgetItem* item: same_name_items)
        {
            if( item->column() == 0){ found = true; }
        }
        if( !found )
        {
            int new_row = ui->listRosTopics->rowCount();
            ui->listRosTopics->setRowCount( new_row+1 );

            // order IS important, don't change it
            ui->listRosTopics->setItem(new_row, 1, new QTableWidgetItem( type_name ));
            ui->listRosTopics->setItem(new_row, 0, new QTableWidgetItem( topic_name ));

            if(_default_selected_topics.contains(topic_name) )
            {
                ui->listRosTopics->selectRow(new_row);
            }
        }
    }

    ui->listRosTopics->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->listRosTopics->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->listRosTopics->sortByColumn(0, Qt::AscendingOrder);
}


DialogSelectRosTopics::~DialogSelectRosTopics()
{
    delete ui;
}

QStringList DialogSelectRosTopics::getSelectedItems()
{
    return _topic_list;
}


QCheckBox *DialogSelectRosTopics::checkBoxUseHeaderStamp()
{
    return ui->checkBoxUseHeaderStamp;
}

QCheckBox* DialogSelectRosTopics::checkBoxUseRenamingRules()
{
    return ui->checkBoxEnableRules;
}


void DialogSelectRosTopics::on_buttonBox_accepted()
{
    QModelIndexList selected_indexes = ui->listRosTopics->selectionModel()->selectedIndexes();
    QString selected_topics;

    foreach(QModelIndex index, selected_indexes)
    {
        if(index.column() == 0){
            _topic_list.push_back( index.data(Qt::DisplayRole).toString() );
            selected_topics.append( _topic_list.back() ).append(" ");
        }
    }
    QSettings settings( "IcarusTechnology", "PlotJuggler");
    settings.setValue("DialogSelectRosTopics.enableRules",    ui->checkBoxEnableRules->isChecked() );
    settings.setValue("DialogSelectRosTopics.useHeaderStamp", ui->checkBoxUseHeaderStamp->isChecked() );
    settings.setValue("DialogSelectRosTopics.geometry", saveGeometry());
    settings.setValue("DialogSelectRosTopics.selectedItems", selected_topics );
}

void DialogSelectRosTopics::on_listRosTopics_itemSelectionChanged()
{
    QModelIndexList indexes = ui->listRosTopics->selectionModel()->selectedIndexes();

    ui->buttonBox->setEnabled( indexes.size() > 0) ;
}


void DialogSelectRosTopics::on_checkBoxEnableRules_toggled(bool checked)
{
    ui->pushButtonEditRules->setEnabled( checked );
}

void DialogSelectRosTopics::on_pushButtonEditRules_pressed()
{
    RuleEditing* rule_editing = new RuleEditing(this);
    rule_editing->exec();
}

void DialogSelectRosTopics::closeEvent(QCloseEvent *event)
{
    QSettings settings( "IcarusTechnology", "PlotJuggler");
    settings.setValue("DialogSelectRosTopics.geometry", saveGeometry());
}

nonstd::optional<double> FlatContainedContainHeaderStamp(const RosIntrospection::ROSTypeFlat &flat_container)
{
    const char* ID = "/header/stamp";
    const int renamed_count = flat_container.renamed_value.size();
    const int OFF = strlen(ID);

    // cache the previous result
    static std::map<const RosIntrospection::ROSTypeFlat*,int> first_indexes;

    int first_index = first_indexes[&flat_container];

    if( first_index >= 0 && first_index < renamed_count)
    {
        const RosIntrospection::SString& field_name = flat_container.renamed_value[first_index].first;
        if( field_name.size() > OFF &&
            strcmp( &field_name.data()[ field_name.size() -OFF], ID) == 0)
        {
            return flat_container.renamed_value[first_index].second;
        }
    }

    for(int i=0; i< renamed_count; i++ )
    {
        if( i == first_index ) continue;

        const RosIntrospection::SString& field_name = flat_container.renamed_value[i].first;
        if( field_name.size() > OFF &&
            strcmp( &field_name.data()[ field_name.size() -OFF], ID) == 0)
        {
            first_indexes[&flat_container] = i;
            return flat_container.renamed_value[i].second;
        }
    }
    return nonstd::optional<double>();
}
