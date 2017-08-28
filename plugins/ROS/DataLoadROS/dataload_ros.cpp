#include "dataload_ros.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QApplication>
#include <QProgressDialog>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QProcess>
#include <rosbag/view.h>
#include <sys/sysinfo.h>

#include "../dialog_select_ros_topics.h"
#include "../shape_shifter_factory.hpp"
#include "../rule_editing.h"


DataLoadROS::DataLoadROS()
{
    _extensions.push_back( "bag");
}

const std::vector<const char*> &DataLoadROS::compatibleFileExtensions() const
{
    return _extensions;
}

size_t getAvailableRAM()
{
    struct sysinfo info;
    sysinfo(&info);
    return info.freeram;
}

PlotDataMap DataLoadROS::readDataFromFile(const QString &file_name,
                                          QString &load_configuration  )
{
    using namespace RosIntrospection;

    std::vector<std::pair<QString,QString>> all_topics;
    PlotDataMap plot_map;

    try{
        _bag.close();
        _bag.open( file_name.toStdString(), rosbag::bagmode::Read );
    }
    catch( rosbag::BagException&  ex)
    {
        QMessageBox::warning(0, tr("Error"),
                             QString("rosbag::open thrown an exception:\n")+
                             QString(ex.what()) );
        return plot_map;
    }

    rosbag::View bag_view ( _bag, ros::TIME_MIN, ros::TIME_MAX, true );
    std::vector<const rosbag::ConnectionInfo *> connections = bag_view.getConnections();

    std::map<std::string,ROSType> rostype;

    for(unsigned i=0;  i<connections.size(); i++)
    {
        const auto&  topic      =  connections[i]->topic;
        const auto&  md5sum     =  connections[i]->md5sum;
        const auto&  datatype  =  connections[i]->datatype;
        const auto&  definition =  connections[i]->msg_def;

        all_topics.push_back( std::make_pair(QString( topic.c_str()), QString( datatype.c_str()) ) );
        RosIntrospectionFactory::get().registerMessage(topic, md5sum, datatype, definition);
        rostype.insert( std::make_pair(datatype, ROSType(datatype)) );
    }

    int count = 0;

    //----------------------------------
    QStringList default_topic_names = load_configuration.split(' ',QString::SkipEmptyParts);
    load_configuration.clear();

    DialogSelectRosTopics* dialog = new DialogSelectRosTopics( all_topics, default_topic_names );

    std::set<std::string> topic_selected;

    if( dialog->exec() == static_cast<int>(QDialog::Accepted) )
    {
        const auto& selected_items = dialog->getSelectedItems();
        for(const auto& item: selected_items)
        {
            std::string ss_topic = item.toStdString();
            topic_selected.insert( ss_topic );

            // change the names in load_configuration
            load_configuration.append( item ).append(" ");
        }
        // load the rules
        if( dialog->checkBoxUseRenamingRules()->isChecked())
        {
            _rules = RuleEditing::getRenamingRules();
        }
        else{
            _rules.clear();
        }
    }

    //-----------------------------------
    QProgressDialog progress_dialog;
    progress_dialog.setLabelText("Loading... please wait");
    progress_dialog.setWindowModality( Qt::ApplicationModal );

    rosbag::View bag_view_selected ( true );
    bag_view_selected.addQuery( _bag, [&topic_selected](rosbag::ConnectionInfo const* connection)
    {
        return topic_selected.find( connection->topic ) != topic_selected.end();
    } );
    progress_dialog.setRange(0, bag_view_selected.size()-1);
    progress_dialog.show();

    QElapsedTimer timer;
    timer.start();

    ROSTypeFlat flat_container;

    SString topicname_SS;
    std::vector<uint8_t> buffer;

    for(rosbag::MessageInstance msg_instance: bag_view_selected )
    {
        const std::string& topic  = msg_instance.getTopic();

        // WORKAROUND. There are some problems related to renaming when the character / is
        // used as prefix. We will remove that here.
        if( topic.at(0) == '/' )
            topicname_SS.assign( topic.data() +1,  topic.size()-1 );
        else
            topicname_SS.assign( topic.data(),  topic.size() );

        const auto& datatype = msg_instance.getDataType();
        const auto msg_size  = msg_instance.size();
        buffer.resize(msg_size);

        if( count++ %100 == 0)
        {
            progress_dialog.setValue( count );
            QApplication::processEvents();

            if( progress_dialog.wasCanceled() ) {
                return PlotDataMap();
            }
        }

        ros::serialization::OStream stream(buffer.data(), buffer.size());

        // this single line takes almost the entire time of the loop
        msg_instance.write(stream);

        auto typelist = RosIntrospectionFactory::get().getRosTypeList( topic );
        if( !typelist )
        {
            throw std::runtime_error("Can't retrieve the ROSTypeList from RosIntrospectionFactory");
        }
        buildRosFlatType( *typelist, rostype[datatype], topicname_SS, buffer.data(), &flat_container);

        static RenamedValues renamed_value;
 
        auto rules_it = _rules.find(datatype);
        if(rules_it != _rules.end())
        {
            applyNameTransform( rules_it->second, flat_container, renamed_value );
        }
        else{
            applyNameTransform( std::vector<SubstitutionRule>(), flat_container, renamed_value );
        }

        // apply time offsets
        double msg_time = 0;

        if(dialog->checkBoxUseHeaderStamp()->isChecked() == false)
        {
            msg_time = msg_instance.getTime().toSec();
        }
        else{
            auto offset = FlatContainedContainHeaderStamp(renamed_value);
            if(offset){
                msg_time = offset.value();
            }
            else{
                msg_time = msg_instance.getTime().toSec();
            }
        }

        for(const auto& it: renamed_value )
        {
            const std::string& field_name = it.first;

            auto plot_pair = plot_map.numeric.find( field_name );
            if( plot_pair == plot_map.numeric.end() )
            {
                PlotDataPtr temp(new PlotData(field_name.data()));
                auto res = plot_map.numeric.insert( std::make_pair(field_name, temp ) );
                plot_pair = res.first;
            }

            PlotDataPtr& plot_data = plot_pair->second;
            const RosIntrospection::VarNumber& var_value = it.second;
            plot_data->pushBack( PlotData::Point(msg_time, var_value.convert<double>() ));
        } //end of for renamed_value

        //-----------------------------------------
        // adding raw serialized topic for future uses.
        {
            auto plot_pair = plot_map.user_defined.find( topic );

            if( plot_pair == plot_map.user_defined.end() )
            {
                PlotDataAnyPtr temp(new PlotDataAny(topic.c_str()));
                auto res = plot_map.user_defined.insert( std::make_pair( topic, temp ) );
                plot_pair = res.first;
            }
            PlotDataAnyPtr& plot_raw = plot_pair->second;
            plot_raw->pushBack( PlotDataAny::Point(msg_time, nonstd::any(std::move(msg_instance)) ));
        }
    }

    qDebug() << "The loading operation took" << timer.elapsed() << "milliseconds";
    return plot_map;
}


DataLoadROS::~DataLoadROS()
{

}


