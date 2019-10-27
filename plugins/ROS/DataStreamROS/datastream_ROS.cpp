#include "datastream_ROS.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <thread>
#include <mutex>
#include <chrono>
#include <thread>
#include <QProgressDialog>
#include <QtGlobal>
#include <QApplication>
#include <QProcess>
#include <QCheckBox>
#include <QSettings>
#include <QFileDialog>
#include <ros/callback_queue.h>
#include <rosbag/bag.h>
#include <topic_tools/shape_shifter.h>
#include <ros/transport_hints.h>

#include "../dialog_select_ros_topics.h"
#include "../rule_editing.h"
#include "../qnodedialog.h"
#include "../shape_shifter_factory.hpp"

DataStreamROS::DataStreamROS():
    DataStreamer(),
    _node(nullptr),
    _destination_data(nullptr),
    _action_saveIntoRosbag(nullptr),
    _prev_clock_time(0)
{
    _running = false;
    _periodic_timer = new QTimer();
    connect( _periodic_timer, &QTimer::timeout,
             this, &DataStreamROS::timerCallback);

    loadDefaultSettings();
}

void DataStreamROS::topicCallback(const topic_tools::ShapeShifter::ConstPtr& msg,
                                  const std::string &topic_name)
{
    if( !_running ){
        return;
    }

    using namespace RosIntrospection;
    const auto&  md5sum     =  msg->getMD5Sum();
    const auto&  datatype   =  msg->getDataType();
    const auto&  definition =  msg->getMessageDefinition() ;

    // register the message type
    _ros_parser.registerSchema( topic_name, md5sum,
                                RosIntrospection::ROSType(datatype),
                                definition);

    RosIntrospectionFactory::registerMessage(topic_name, md5sum, datatype, definition );

    //------------------------------------

    // it is more efficient to recycle this elements
    static std::vector<uint8_t> buffer;
    
    buffer.resize( msg->size() );
    
    ros::serialization::OStream stream(buffer.data(), buffer.size());
    msg->write(stream);

    double msg_time = ros::Time::now().toSec();

    if( msg_time < _prev_clock_time )
    {
        // clean
        for (auto& it: dataMap().numeric ) {
            it.second.clear();
            auto dst = _destination_data->numeric.find(it.first);
            if( dst != _destination_data->numeric.end()){
                dst->second.clear();
            }
        }
        for (auto& it: dataMap().user_defined ){
            it.second.clear();
            auto dst = _destination_data->user_defined.find(it.first);
            if( dst != _destination_data->user_defined.end()){
                dst->second.clear();
            }
        }
    }
    _prev_clock_time = msg_time;

    MessageRef buffer_view( buffer );
    _ros_parser.pushMessageRef( topic_name, buffer_view, msg_time );

    std::lock_guard<std::mutex> lock( mutex() );
    const std::string prefixed_topic_name = _ros_parser.getPrefixedTopic(topic_name);
   // const std::string prefixed_topic_name = _prefix + topic_name;

    // adding raw serialized msg for future uses.
    // do this before msg_time normalization
    {
        auto plot_pair = dataMap().user_defined.find( topic_name );
        if( plot_pair == dataMap().user_defined.end() )
        {
            plot_pair = dataMap().addUserDefined( topic_name );
        }
        PlotDataAny& user_defined_data = plot_pair->second;
        user_defined_data.pushBack( PlotDataAny::Point(msg_time, nonstd::any(std::move(buffer)) ));
    }

    _ros_parser.extractData(dataMap(),{});

    //------------------------------
    {
        int& index = _msg_index[topic_name];
        index++;
        const std::string key = prefixed_topic_name + ("/_MSG_INDEX_") ;
        auto index_it = dataMap().numeric.find(key);
        if( index_it == dataMap().numeric.end())
        {
            index_it = dataMap().addNumeric( key );
        }
        index_it->second.pushBack( PlotData::Point(msg_time, index) );
    }
}

void DataStreamROS::extractInitialSamples()
{
    using namespace std::chrono;
    milliseconds wait_time_ms(1000);

    QProgressDialog progress_dialog;
    progress_dialog.setLabelText( "Collecting ROS topic samples to understand data layout. ");
    progress_dialog.setRange(0, wait_time_ms.count());
    progress_dialog.setAutoClose(true);
    progress_dialog.setAutoReset(true);

    progress_dialog.show();

    auto start_time = system_clock::now();

    while ( system_clock::now() - start_time < (wait_time_ms) )
    {
        ros::getGlobalCallbackQueue()->callAvailable(ros::WallDuration(0.1));
        int i = duration_cast<milliseconds>(system_clock::now() - start_time).count() ;
        progress_dialog.setValue( i );
        QApplication::processEvents();
        if( progress_dialog.wasCanceled() )
        {
            break;
        }
    }

    if( progress_dialog.wasCanceled() == false )
    {
        progress_dialog.cancel();
    }
}

void DataStreamROS::timerCallback()
{
    if( _running && ros::master::check() == false
            && !_roscore_disconnection_already_notified)
    {
        auto ret = QMessageBox::warning(nullptr,
                                        tr("Disconnected!"),
                                        tr("The roscore master cannot be detected.\n\n"
                                           "Do you want to try reconnecting to it? \n\n"
                                           "NOTE: if you select CONTINUE, you might need"
                                           " to stop and restart this plugin."),
                                        tr("Stop Plugin"),
                                        tr("Try reconnect"),
                                        tr("Continue"),
                                        0);
        _roscore_disconnection_already_notified = ( ret == 2 );
        if( ret == 1 )
        {
            this->shutdown();
            _node =  RosManager::getNode();

            if( !_node ){
                emit connectionClosed();
                return;
            }
            subscribe();

            _running = true;
            _spinner = std::make_shared<ros::AsyncSpinner>(1);
            _spinner->start();
            _periodic_timer->start();
        }
        else if( ret == 0)
        {
            this->shutdown();
            emit connectionClosed();
        }
    }
}

void DataStreamROS::saveIntoRosbag(const PlotDataMapRef& data)
{

    if( data.user_defined.empty()){
        QMessageBox::warning(nullptr, tr("Warning"), tr("Your buffer is empty. Nothing to save.\n") );
        return;
    }

    QFileDialog saveDialog;
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix("bag");
    saveDialog.exec();

    if(saveDialog.result() != QDialog::Accepted || saveDialog.selectedFiles().empty())
    {
        return;
    }

    QString fileName = saveDialog.selectedFiles().first();

    if( fileName.size() > 0)
    {
        rosbag::Bag rosbag(fileName.toStdString(), rosbag::bagmode::Write );

        for (const auto& it: data.user_defined )
        {
            const std::string& topicname = it.first;
            const auto& plotdata = it.second;

            auto registered_msg_type = RosIntrospectionFactory::get().getShapeShifter(topicname);
            if(!registered_msg_type) continue;

            RosIntrospection::ShapeShifter msg;
            msg.morph(registered_msg_type->getMD5Sum(),
                      registered_msg_type->getDataType(),
                      registered_msg_type->getMessageDefinition());

            for (int i=0; i< plotdata.size(); i++)
            {
                const auto& point = plotdata.at(i);
                const PlotDataAny::TimeType msg_time  = point.x;
                const nonstd::any& type_erased_buffer = point.y;

                if(type_erased_buffer.type() != typeid( std::vector<uint8_t> ))
                {
                    // can't cast to expected type
                    continue;
                }

                std::vector<uint8_t> raw_buffer =  nonstd::any_cast<std::vector<uint8_t>>( type_erased_buffer );
                ros::serialization::IStream stream( raw_buffer.data(), raw_buffer.size() );
                msg.read( stream );

                rosbag.write( topicname, ros::Time(msg_time), msg);
            }
        }
        rosbag.close();

        QProcess process;
        QStringList args;
        args << "reindex" << fileName;
        process.start("rosbag" , args);
    }
}


void DataStreamROS::subscribe()
{
    _subscribers.clear();
    {
        boost::function<void(const rosgraph_msgs::Clock::ConstPtr&) > callback;
        callback = [this](const rosgraph_msgs::Clock::ConstPtr& msg) -> void
        {
            this->clockCallback(msg) ;
        };
    }

    for (int i=0; i< _config.selected_topics.size(); i++ )
    {
        const std::string topic_name = _config.selected_topics[i].toStdString();
        boost::function<void(const topic_tools::ShapeShifter::ConstPtr&) > callback;
        callback = [this, topic_name](const topic_tools::ShapeShifter::ConstPtr& msg) -> void
        {
            this->topicCallback(msg, topic_name) ;
        };

        ros::SubscribeOptions ops;
        ops.initByFullCallbackType(topic_name, 1, callback);
        ops.transport_hints = ros::TransportHints().tcpNoDelay();

        _subscribers.insert( {topic_name, _node->subscribe(ops) }  );
    }
}

bool DataStreamROS::start(QStringList* selected_datasources)
{
    _ros_parser.clear();
    if( !_node )
    {
        _node =  RosManager::getNode();
    }

    if( !_node ){
        return false;
    }

    {
        std::lock_guard<std::mutex> lock( mutex() );
        dataMap().numeric.clear();
        dataMap().user_defined.clear();
    }

    using namespace RosIntrospection;

    std::vector<std::pair<QString,QString>> all_topics;
    ros::master::V_TopicInfo topic_infos;
    ros::master::getTopics(topic_infos);
    for (ros::master::TopicInfo topic_info: topic_infos)
    {
        all_topics.push_back(
                    std::make_pair(QString(topic_info.name.c_str()),
                                   QString(topic_info.datatype.c_str()) ) );
    }

    QTimer timer;
    timer.setSingleShot(false);
    timer.setInterval( 1000);
    timer.start();

    DialogSelectRosTopics dialog( all_topics, _config );

    connect( &timer, &QTimer::timeout, [&]()
    {
        all_topics.clear();
        topic_infos.clear();
        ros::master::getTopics(topic_infos);
        for (ros::master::TopicInfo topic_info: topic_infos)
        {
            all_topics.push_back(
                        std::make_pair(QString(topic_info.name.c_str()),
                                       QString(topic_info.datatype.c_str()) ) );
        }
        dialog.updateTopicList(all_topics);
    });

    int res = dialog.exec();

    _config = dialog.getResult();

    timer.stop();

    if( res != QDialog::Accepted || _config.selected_topics.empty() )
    {
        return false;
    }

    saveDefaultSettings();

    _ros_parser.setUseHeaderStamp( _config.use_header_stamp );

    if( _config.use_renaming_rules )
    {
        _ros_parser.addRules( RuleEditing::getRenamingRules() );
    }

    _ros_parser.setMaxArrayPolicy( _config.max_array_size, _config.discard_large_arrays );
    _ros_parser.setTopicTransform(_config.prefix.toStdString(), _config.getRemovePrefixes());
    //-------------------------
    subscribe();
    _running = true;

    extractInitialSamples();

    _spinner = std::make_shared<ros::AsyncSpinner>(1);
    _spinner->start();

    _periodic_timer->setInterval(500);
    _roscore_disconnection_already_notified = false;
    _periodic_timer->start();

    return true;
}

bool DataStreamROS::isRunning() const { return _running; }

void DataStreamROS::shutdown()
{
    _periodic_timer->stop();
    if(_spinner)
    {
        _spinner->stop();
    }
    for(auto& it: _subscribers)
    {
        it.second.shutdown();
    }
    _subscribers.clear();
    _running = false;
    _node.reset();
    _spinner.reset();
}

DataStreamROS::~DataStreamROS()
{
    shutdown();
}

bool DataStreamROS::xmlSaveState(QDomDocument &doc, QDomElement &plugin_elem) const
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

    return true;
}

bool DataStreamROS::xmlLoadState(const QDomElement &parent_element)
{
    QDomElement stamp_elem = parent_element.firstChildElement( "use_header_stamp" );
    _config.use_header_stamp = ( stamp_elem.attribute("value") == "true");

    QDomElement rename_elem = parent_element.firstChildElement( "use_renaming_rules" );
    _config.use_renaming_rules = ( rename_elem.attribute("value") == "true");

    QDomElement discard_elem = parent_element.firstChildElement( "discard_large_arrays" );
    _config.discard_large_arrays = ( discard_elem.attribute("value") == "true");

    QDomElement max_elem = parent_element.firstChildElement( "max_array_size" );
    _config.max_array_size = max_elem.attribute("value").toInt();

    return true;
}

void DataStreamROS::addActionsToParentMenu(QMenu *menu)
{
    _action_saveIntoRosbag = new QAction(QString("Save cached value in a rosbag"), menu);
    menu->addAction( _action_saveIntoRosbag );

    connect( _action_saveIntoRosbag, &QAction::triggered, this, [this]()
    {
        DataStreamROS::saveIntoRosbag( *_destination_data );
    });
}

void DataStreamROS::saveDefaultSettings()
{
    QSettings settings;

    settings.setValue("DataStreamROS/default_topics", _config.selected_topics);
    settings.setValue("DataStreamROS/use_renaming", _config.use_renaming_rules);
    settings.setValue("DataStreamROS/use_header_stamp", _config.use_header_stamp);
    settings.setValue("DataStreamROS/max_array_size", (int)_config.max_array_size);
    settings.setValue("DataStreamROS/discard_large_arrays", _config.discard_large_arrays);
}


void DataStreamROS::loadDefaultSettings()
{
    QSettings settings;
    _config.selected_topics      = settings.value("DataStreamROS/default_topics", false ).toStringList();
    _config.use_header_stamp     = settings.value("DataStreamROS/use_header_stamp", false ).toBool();
    _config.use_renaming_rules   = settings.value("DataStreamROS/use_renaming", true ).toBool();
    _config.max_array_size       = settings.value("DataStreamROS/max_array_size", 100 ).toInt();
    _config.discard_large_arrays = settings.value("DataStreamROS/discard_large_arrays", true ).toBool();
}


