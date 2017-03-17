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
#include <QFileDialog>
#include <ros/callback_queue.h>
#include <rosbag/bag.h>
#include <topic_tools/shape_shifter.h>

#include "../dialog_select_ros_topics.h"
#include "../rule_editing.h"
#include "../qnodedialog.h"
#include "../shape_shifter_factory.hpp"

DataStreamROS::DataStreamROS():
    _action_saveIntoRosbag(nullptr)
{
    _enabled = false;
    _running = false;
    _initial_time = std::numeric_limits<double>::max();

    _use_header_timestamp = true;
    _normalize_time = true;
}

PlotDataMap& DataStreamROS::getDataMap()
{
    return _plot_data;
}

void DataStreamROS::topicCallback(const topic_tools::ShapeShifter::ConstPtr& msg, const std::string &topic_name)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if( !_running ||  !_enabled){
        return;
    }

    using namespace RosIntrospection;

    const auto&  md5sum     =  msg->getMD5Sum();
    const auto&  datatype   =  msg->getDataType();
    const auto&  definition =  msg->getMessageDefinition() ;
    ShapeShifterFactory::getInstance().registerMessage(topic_name, md5sum, datatype, definition);

    // Decode this message time if it is the first time you receive it
    auto it = _ros_type_map.find(datatype);
    if( it == _ros_type_map.end() )
    {
        auto typemap = buildROSTypeMapFromDefinition( datatype,  definition);
        auto ret = _ros_type_map.insert( std::make_pair(datatype, std::move(typemap)));
        it = ret.first;
    }
    const RosIntrospection::ROSTypeList& type_map = it->second;

    //------------------------------------
    std::vector<uint8_t> buffer(msg->size()); // "64 KB ought to be enough for anybody"

    // it is more efficient to recycle ROSTypeFlat
    static ROSTypeFlat flat_container;

    ros::serialization::OStream stream(buffer.data(), buffer.size());
    msg->write(stream);

    SString topicname( topic_name.data(), topic_name.length() );

    buildRosFlatType( type_map, datatype, topicname, buffer.data(), &flat_container);
    applyNameTransform( _rules[datatype], &flat_container );

    SString header_stamp_field( topic_name );
    header_stamp_field.append(".header.stamp");

    double msg_time = 0;

    // detrmine the time offset
    if(_use_header_timestamp == false)
    {
        msg_time = ros::Time::now().toSec();
    }
    else{
        auto offset = FlatContainedContainHeaderStamp(flat_container);
        if(offset){
            msg_time = offset.value();
        }
        else{
            msg_time = ros::Time::now().toSec();
        }
    }

    // adding raw serialized msg for future uses.
    // do this before msg_time normalization
    {
        auto plot_pair = _plot_data.user_defined.find( md5sum );
        if( plot_pair == _plot_data.user_defined.end() )
        {
            PlotDataAnyPtr temp(new PlotDataAny());
            auto res = _plot_data.user_defined.insert( std::make_pair( topic_name, temp ) );
            plot_pair = res.first;
        }
        PlotDataAnyPtr& user_defined_data = plot_pair->second;
        user_defined_data->pushBack( PlotDataAny::Point(msg_time, nonstd::any(std::move(buffer)) ));
    }

    if( _normalize_time )
    {
        _initial_time = std::min( _initial_time, msg_time );
        msg_time -= _initial_time;
    }


    PlotData::asyncPushMutex().lock();
    for(auto& it: flat_container.renamed_value )
    {
        std::string field_name ( it.first.data(), it.first.size());
        double value = it.second;
        auto plot = _plot_data.numeric[field_name];
        plot->pushBackAsynchronously( PlotData::Point(msg_time, value));
    }
    PlotData::asyncPushMutex().unlock();
}

void DataStreamROS::extractInitialSamples()
{
    int wait_time = 2;

    QProgressDialog progress_dialog;
    progress_dialog.setLabelText( "Collecting ROS topic samples to understand data layout. ");
    progress_dialog.setRange(0, wait_time*1000);
    progress_dialog.setAutoClose(true);
    progress_dialog.setAutoReset(true);

    progress_dialog.show();

    using namespace std::chrono;
    auto start_time = system_clock::now();

    _enabled = true;
    while ( system_clock::now() - start_time < seconds(wait_time) )
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
    _enabled = false;

    if( progress_dialog.wasCanceled() == false )
    {
        progress_dialog.cancel();
    }
}

void DataStreamROS::saveIntoRosbag()
{
    if( _plot_data.user_defined.empty()){
        QMessageBox::warning(0, tr("Warning"), tr("Your buffer is empty. Nothing to save.\n") );
        return;
    }

    QFileDialog saveDialog;
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix("bag");
    saveDialog.exec();
    QString fileName = saveDialog.selectedFiles().first();

    if( fileName.size() > 0)
    {
        rosbag::Bag rosbag(fileName.toStdString(), rosbag::bagmode::Write );

        for (auto it: _plot_data.user_defined )
        {
            const std::string& topicname = it.first;
            const PlotDataAnyPtr& plotdata = it.second;

            auto registered_msg_type = ShapeShifterFactory::getInstance().getMessage(topicname);
            if(!registered_msg_type) continue;

            RosIntrospection::ShapeShifter msg;
            msg.morph(registered_msg_type.value()->getMD5Sum(),
                      registered_msg_type.value()->getDataType(),
                      registered_msg_type.value()->getMessageDefinition());

            for (int i=0; i< plotdata->size(); i++)
            {
                const auto& point = plotdata->at(i);
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
        process.start("rosbag" ,args);
    }
}


bool DataStreamROS::start()
{
    _plot_data.numeric.clear();
    _plot_data.user_defined.clear();
    _initial_time = std::numeric_limits<double>::max();

    _node = getGlobalRosNode();
    if( !_node )
    {
        return false;
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

    DialogSelectRosTopics dialog(all_topics, QStringList(), 0 );
    int res = dialog.exec();

    QStringList topic_selected = dialog.getSelectedItems();

    if( res != QDialog::Accepted || topic_selected.empty() )
    {
        return false;
    }

    // load the rules
    if( dialog.checkBoxUseRenamingRules()->isChecked() ){
        _rules = RuleEditing::getRenamingRules();
    }
    _normalize_time       = dialog.checkBoxNormalizeTime()->isChecked();
    _use_header_timestamp = dialog.checkBoxUseHeaderStamp()->isChecked();
    //-------------------------

    ros::start(); // needed because node will go out of scope

    _subscribers.clear();
    for (int i=0; i<topic_selected.size(); i++ )
    {
        auto topic_name = topic_selected.at(i).toStdString();
        boost::function<void(const topic_tools::ShapeShifter::ConstPtr&) > callback;
        callback = boost::bind( &DataStreamROS::topicCallback, this, _1, topic_name ) ;
        _subscribers.push_back( _node->subscribe( topic_name, 0,  callback)  );
    }

    _running = true;

    extractInitialSamples();

    _thread = std::thread([this](){ this->updateLoop();} );

    return true;
}

void DataStreamROS::enableStreaming(bool enable) { _enabled = enable; }

bool DataStreamROS::isStreamingEnabled() const { return _enabled; }


void DataStreamROS::shutdown()
{
    if( _running ){
        _running = false;
        _thread.join();
        _node.reset();
    }

    for(ros::Subscriber& sub: _subscribers)
    {
        sub.shutdown();
    }
    if(ros::isStarted() )
    {
        ros::shutdown(); // explicitly needed since we use ros::start();;
        ros::waitForShutdown();
    }

    _subscribers.clear();
}

DataStreamROS::~DataStreamROS()
{
    shutdown();
}

void DataStreamROS::setMenu(QMenu *menu)
{
    _menu = menu;

    _action_saveIntoRosbag = new QAction(QString("Save cached value in a rosbag"), _menu);
    QIcon iconSave;
    iconSave.addFile(QStringLiteral(":/icons/resources/filesave@2x.png"), QSize(26, 26), QIcon::Normal, QIcon::Off);
    _action_saveIntoRosbag->setIcon(iconSave);

    _menu->addAction( _action_saveIntoRosbag );
    _menu->addSeparator();

    connect( _action_saveIntoRosbag, SIGNAL(triggered()), this, SLOT(saveIntoRosbag()) );
}


void DataStreamROS::updateLoop()
{
    while (ros::ok() && _running)
    {
        //  ros::getGlobalCallbackQueue()->callAvailable(ros::WallDuration(0.2));
        // _node->spin();
        ros::spinOnce();
    }
    ros::shutdown();
}
