#include "dataload_ros.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QDebug>
#include <QApplication>
#include <QProgressDialog>
#include <QFileInfo>
#include <QProcess>
#include <rosbag/view.h>
#include <QSettings>
#include <QElapsedTimer>

#include "../dialog_select_ros_topics.h"
#include "../shape_shifter_factory.hpp"
#include "../rule_editing.h"
#include "../dialog_with_itemlist.h"

DataLoadROS::DataLoadROS()
{
    _extensions.push_back( "bag");
    loadDefaultSettings();
}

void StrCat(const std::string& a, const std::string& b,  std::string& out)
{
    out.clear();
    out.reserve(a.size() + b.size());
    out.assign(a);
    out.append(b);
}

const std::vector<const char*> &DataLoadROS::compatibleFileExtensions() const
{
    return _extensions;
}

std::vector<std::pair<QString,QString>> DataLoadROS::getAndRegisterAllTopics()
{
    std::vector<std::pair<QString,QString>> all_topics;
    rosbag::View bag_view ( *_bag, ros::TIME_MIN, ros::TIME_MAX, true );

    RosIntrospectionFactory::reset();

    bool ignoreAll = false;

    for(auto& conn: bag_view.getConnections() )
    {
        const auto&  topic      =  conn->topic;
        const auto&  md5sum     =  conn->md5sum;
        const auto&  datatype   =  conn->datatype;
        const auto&  definition =  conn->msg_def;

        all_topics.push_back( std::make_pair(QString( topic.c_str()), QString( datatype.c_str()) ) );
        try {
            _ros_parser.registerSchema(
                    topic, md5sum, RosIntrospection::ROSType(datatype), definition);
            RosIntrospectionFactory::registerMessage(topic, md5sum, datatype, definition);
        }
        catch(std::exception &ex)
        {
            // there was a problem with this topic
            // a real life problem example can be found here:
            // https://github.com/rosjava/rosjava_bootstrap/issues/16
            all_topics.pop_back();

            if (ignoreAll) {
                // this is not the first error with this load and the
                // user has accepted to ignore all errors
                continue;
            }

            // prompt user to abort or continue
            QMessageBox msgBox(nullptr);
            msgBox.setWindowTitle("ROS bag error");
            msgBox.setText(QString("Topic ") +
                           QString(topic.c_str()) +
                           QString(": ") +
                           QString(ex.what()));

            QPushButton* buttonCancel = msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
            QPushButton* buttonIgnore = msgBox.addButton(tr("Ignore"), QMessageBox::YesRole);
            QPushButton* buttonIgnoreAll = msgBox.addButton(tr("Ignore all"), QMessageBox::AcceptRole);
            msgBox.setDefaultButton(buttonIgnoreAll);
            msgBox.exec();
            if( msgBox.clickedButton() == buttonCancel)
            {
                // abort the file loading
                throw;
            }
            if( msgBox.clickedButton() == buttonIgnoreAll)
            {
                // accept this and all future errors for this load
                ignoreAll = true;
            }
        }
    }
    return all_topics;
}


bool DataLoadROS::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_map)
{
    if( _bag ) _bag->close();

    _bag = std::make_shared<rosbag::Bag>();
    _ros_parser.clear();

    try{
        _bag->open( info->filename.toStdString(), rosbag::bagmode::Read );
    }
    catch( rosbag::BagException&  ex)
    {
        QMessageBox::warning(nullptr, tr("Error"),
                             QString("rosbag::open thrown an exception:\n")+
                             QString(ex.what()) );
        return false;
    }

    auto all_topics = getAndRegisterAllTopics();

    //----------------------------------

    if( info->plugin_config.hasChildNodes() )
    {
        xmlLoadState( info->plugin_config.firstChildElement() );
    }
    if (!info->prefix.isEmpty())
    {
      _config.prefix = info->prefix;
      info->prefix = "";
    }

    if( ! info->selected_datasources.empty() )
    {
            _config.selected_topics =   info->selected_datasources;
    }
    else{
        DialogSelectRosTopics* dialog = new DialogSelectRosTopics( all_topics, _config );

        if( dialog->exec() != static_cast<int>(QDialog::Accepted) )
        {
            return false;
        }
        _config = dialog->getResult();
    }

    saveDefaultSettings();

    _ros_parser.setUseHeaderStamp( _config.use_header_stamp );
    _ros_parser.setMaxArrayPolicy( _config.max_array_size, _config.discard_large_arrays );
    _ros_parser.setTopicTransform( _config.prefix.toStdString(), _config.getRemovePrefixes());
    if( _config.use_renaming_rules )
    {
        _ros_parser.addRules( RuleEditing::getRenamingRules() );
    }

    //-----------------------------------
    std::set<std::string> topic_selected;
    for(const auto& topic: _config.selected_topics)
    {
        topic_selected.insert( topic.toStdString() );
    }

    QProgressDialog progress_dialog;
    progress_dialog.setLabelText("Loading... please wait");
    progress_dialog.setWindowModality( Qt::ApplicationModal );

    rosbag::View bag_view ( *_bag );

    progress_dialog.setRange(0, bag_view.size()-1);
    progress_dialog.show();

    std::vector<uint8_t> buffer;

    int msg_count = 0;

    QElapsedTimer timer;
    timer.start();


    PlotDataAny& plot_consecutive = plot_map.addUserDefined( "__consecutive_message_instances__" )->second;

    for(const rosbag::MessageInstance& msg_instance: bag_view)
    {

        const std::string& topic_name  = msg_instance.getTopic();
        double msg_time = msg_instance.getTime().toSec();
        auto data_point = PlotDataAny::Point(msg_time, nonstd::any(msg_instance) );
        plot_consecutive.pushBack( data_point );

        const std::string* key_ptr = &topic_name ;

        auto plot_pair = plot_map.user_defined.find( *key_ptr );
        if( plot_pair == plot_map.user_defined.end() )
        {
            plot_pair = plot_map.addUserDefined( *key_ptr );
        }
        PlotDataAny& plot_raw = plot_pair->second;
        plot_raw.pushBack( data_point );
        //------------------------------------------
        if( msg_count++ %100 == 0)
        {
            progress_dialog.setValue( msg_count );
            QApplication::processEvents();

            if( progress_dialog.wasCanceled() ) {
                return false;
            }
        }
        //------------------------------------------
        if( topic_selected.find( topic_name ) == topic_selected.end() )
        {
            continue;
        }

        const size_t msg_size  = msg_instance.size();
        buffer.resize(msg_size);

        ros::serialization::OStream stream(buffer.data(), buffer.size());
        msg_instance.write(stream);
        MessageRef buffer_view( buffer );
        _ros_parser.pushMessageRef( topic_name, buffer_view, msg_time );
    }

    _ros_parser.extractData(plot_map, "");

    qDebug() << "The loading operation took" << timer.elapsed() << "milliseconds";

    info->selected_datasources = _config.selected_topics;
    return true;
}

DataLoadROS::~DataLoadROS()
{

}

bool DataLoadROS::xmlSaveState(QDomDocument &doc, QDomElement &plugin_elem) const
{
    QDomElement stamp_elem = doc.createElement("use_header_stamp");
    stamp_elem.setAttribute("value", _config.use_header_stamp ? "true" : "false");
    plugin_elem.appendChild( stamp_elem );

    QDomElement rename_elem = doc.createElement("use_renaming_rules");
    rename_elem.setAttribute("value", _config.use_renaming_rules ? "true" : "false");
    plugin_elem.appendChild( rename_elem );

    QDomElement discard_elem = doc.createElement("discard_large_arrays");
    discard_elem.setAttribute("value", _config.discard_large_arrays ? "true" : "false");
    plugin_elem.appendChild( discard_elem );

    QDomElement max_elem = doc.createElement("max_array_size");
    max_elem.setAttribute("value", QString::number(_config.max_array_size));
    plugin_elem.appendChild( max_elem );

    QDomElement prefix_elem = doc.createElement("prefix");
    prefix_elem.setAttribute("value", _config.prefix);
    plugin_elem.appendChild( prefix_elem );

    QDomElement remove_prefixes_elem = doc.createElement("remove_prefixes");
    remove_prefixes_elem.setAttribute("value", _config.rm_prefixes);
    plugin_elem.appendChild( remove_prefixes_elem );

    return true;
}

bool DataLoadROS::xmlLoadState(const QDomElement &parent_element)
{
    QDomElement stamp_elem = parent_element.firstChildElement( "use_header_stamp" );
    _config.use_header_stamp = ( stamp_elem.attribute("value") == "true");

    QDomElement rename_elem = parent_element.firstChildElement( "use_renaming_rules" );
    _config.use_renaming_rules = ( rename_elem.attribute("value") == "true");

    QDomElement discard_elem = parent_element.firstChildElement( "discard_large_arrays" );
    _config.discard_large_arrays = ( discard_elem.attribute("value") == "true");

    QDomElement max_elem = parent_element.firstChildElement( "max_array_size" );
    _config.max_array_size = max_elem.attribute("value").toInt();

    QDomElement prefix_elem = parent_element.firstChildElement( "prefix" );
    _config.prefix = prefix_elem.attribute("value");

    QDomElement remove_prefix_elem = parent_element.firstChildElement( "remove_prefixes" );
    _config.rm_prefixes = remove_prefix_elem.attribute("value");

    return true;
}

void DataLoadROS::saveDefaultSettings()
{
    QSettings settings;

    settings.setValue("DataLoadROS/default_topics", _config.selected_topics);
    settings.setValue("DataLoadROS/use_renaming", _config.use_renaming_rules);
    settings.setValue("DataLoadROS/use_header_stamp", _config.use_header_stamp);
    settings.setValue("DataLoadROS/max_array_size", (int)_config.max_array_size);
    settings.setValue("DataLoadROS/discard_large_arrays", _config.discard_large_arrays);
    settings.setValue("DataLoadROS/prefix", _config.prefix);
    settings.setValue("DataLoadROS/remove_prefixes", _config.rm_prefixes);
}


void DataLoadROS::loadDefaultSettings()
{
    QSettings settings;

    _config.selected_topics      = settings.value("DataLoadROS/default_topics" ).toStringList();
    _config.use_header_stamp     = settings.value("DataLoadROS/use_header_stamp", false ).toBool();
    _config.use_renaming_rules   = settings.value("DataLoadROS/use_renaming", true ).toBool();
    _config.max_array_size       = settings.value("DataLoadROS/max_array_size", 100 ).toInt();
    _config.discard_large_arrays = settings.value("DataLoadROS/discard_large_arrays", true ).toBool();
    _config.prefix               = settings.value("DataLoadROS/prefix", "" ).toString();
    _config.rm_prefixes          = settings.value("DataLoadROS/remove_prefixes", "" ).toString();
}

