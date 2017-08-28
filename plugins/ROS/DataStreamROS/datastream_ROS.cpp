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
    _action_saveIntoRosbag(nullptr),
    _node(nullptr)
{
    _enabled = false;
    _running = false;
    _initial_time = std::numeric_limits<double>::max();
    _use_header_timestamp = true;
}

PlotDataMap& DataStreamROS::getDataMap()
{
    return _plot_data;
}

void DataStreamROS::topicCallback(const topic_tools::ShapeShifter::ConstPtr& msg, const std::string &topic_name)
{
    if( !_running ||  !_enabled){
        return;
    }

    using namespace RosIntrospection;

    const auto&  md5sum     =  msg->getMD5Sum();
    const auto&  datatype   =  msg->getDataType();
    const auto&  definition =  msg->getMessageDefinition() ;

    // register the message type
    RosIntrospectionFactory::get().registerMessage(topic_name, md5sum, datatype, definition);

    const RosIntrospection::ROSTypeList* type_map = RosIntrospectionFactory::get().getRosTypeList(topic_name);
    if( !type_map )
    {
        throw std::runtime_error("Can't retrieve the ROSTypeList from RosIntrospectionFactory");
    }

    //------------------------------------
    std::vector<uint8_t> buffer(msg->size());

    // it is more efficient to recycle ROSTypeFlat
    static ROSTypeFlat flat_container;

    ros::serialization::OStream stream(buffer.data(), buffer.size());
    msg->write(stream);

    SString topicname_SS( topic_name.data(), topic_name.length() );
    // WORKAROUND. There are some problems related to renaming when the character / is
    // used as prefix. We will remove that here.
    if( topicname_SS.at(0) == '/' ) topicname_SS = SString( topic_name.data() +1,  topic_name.size()-1 );

    buildRosFlatType( *type_map, datatype, topicname_SS, buffer.data(), &flat_container);
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
            PlotDataAnyPtr temp(new PlotDataAny(topic_name.c_str()));
            auto res = _plot_data.user_defined.insert( std::make_pair( topic_name, temp ) );
            plot_pair = res.first;
        }
        PlotDataAnyPtr& user_defined_data = plot_pair->second;
        user_defined_data->pushBack( PlotDataAny::Point(msg_time, nonstd::any(std::move(buffer)) ));
    }

    PlotData::asyncPushMutex().lock();
    for(auto& it: flat_container.renamed_value )
    {
        const std::string& field_name ( it.first.data() );
        const RosIntrospection::VarNumber& var_value = it.second;
        const double value = var_value.convertToDouble();
        auto plot_it = _plot_data.numeric.find(field_name);
        if( plot_it == _plot_data.numeric.end())
        {
            auto res =   _plot_data.numeric.insert(
                        std::make_pair( field_name, std::make_shared<PlotData>(field_name.c_str()) ));
            plot_it = res.first;
        }
        plot_it->second->pushBackAsynchronously( PlotData::Point(msg_time, value));
    }

    //------------------------------
    {
        int& index = _msg_index[topic_name];
        index++;
        const std::string index_name = topic_name + ("/_MSG_INDEX_") ;
        auto index_it = _plot_data.numeric.find(index_name);
        if( index_it == _plot_data.numeric.end())
        {
            auto res = _plot_data.numeric.insert(
                        std::make_pair( index_name, std::make_shared<PlotData>(index_name.c_str()) ));
            index_it = res.first;
        }
        index_it->second->pushBackAsynchronously( PlotData::Point(msg_time, index) );
    }
    PlotData::asyncPushMutex().unlock();
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

    _enabled = true;
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

    if(saveDialog.result() != QDialog::Accepted || saveDialog.selectedFiles().empty())
    {
        return;
    }

    QString fileName = saveDialog.selectedFiles().first();

    if( fileName.size() > 0)
    {
        rosbag::Bag rosbag(fileName.toStdString(), rosbag::bagmode::Write );

        for (auto it: _plot_data.user_defined )
        {
            const std::string& topicname = it.first;
            const PlotDataAnyPtr& plotdata = it.second;

            auto registered_msg_type = RosIntrospectionFactory::get().getShapeShifter(topicname);
            if(!registered_msg_type) continue;

            RosIntrospection::ShapeShifter msg;
            msg.morph(registered_msg_type->getMD5Sum(),
                      registered_msg_type->getDataType(),
                      registered_msg_type->getMessageDefinition());

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


bool DataStreamROS::start(QString& default_configuration)
{
    if( !_node )
    {
        _node =  RosManager::getNode();
    }

    if( !_node ){
        return false;
    }

    _plot_data.numeric.clear();
    _plot_data.user_defined.clear();
    _initial_time = std::numeric_limits<double>::max();

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

    QStringList default_topics = default_configuration.split(' ', QString::SkipEmptyParts);

    QTimer timer;
    timer.setSingleShot(false);
    timer.setInterval( 1000);
    timer.start();

    DialogSelectRosTopics dialog(all_topics, default_topics );

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

    timer.stop();

    QStringList topic_selected = dialog.getSelectedItems();
    std::vector<std::string> std_topic_selected;

    if( res != QDialog::Accepted || topic_selected.empty() )
    {
        return false;
    }

    default_configuration.clear();
    for (const QString& topic :topic_selected )
    {
        default_configuration.append(topic).append(" ");
        std_topic_selected.push_back( topic.toStdString() );
    }

    // load the rules
    if( dialog.checkBoxUseRenamingRules()->isChecked() ){
        _rules = RuleEditing::getRenamingRules();
    }

    _use_header_timestamp = dialog.checkBoxUseHeaderStamp()->isChecked();
    //-------------------------

    _subscribers.clear();

    for (int i=0; i<topic_selected.size(); i++ )
    {
        boost::function<void(const topic_tools::ShapeShifter::ConstPtr&) > callback;
        const std::string& topic_name = std_topic_selected[i];
        callback = [this, topic_name](const topic_tools::ShapeShifter::ConstPtr& msg) -> void
        {
            this->topicCallback(msg, topic_name) ;
        };
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
    }

    for(ros::Subscriber& sub: _subscribers)
    {
        sub.shutdown();
    }
    _subscribers.clear();
}

DataStreamROS::~DataStreamROS()
{
    shutdown();
}

void DataStreamROS::setParentMenu(QMenu *menu)
{
    _menu = menu;

    _action_saveIntoRosbag = new QAction(QString("Save cached value in a rosbag"), _menu);
    QIcon iconSave;
    iconSave.addFile(QStringLiteral(":/icons/resources/filesave@2x.png"), QSize(26, 26));
    _action_saveIntoRosbag->setIcon(iconSave);

    _menu->addAction( _action_saveIntoRosbag );

    connect( _action_saveIntoRosbag, &QAction::triggered, this, &DataStreamROS::saveIntoRosbag );
}


void DataStreamROS::updateLoop()
{
    while (ros::ok() && _running)
    {
        ros::spinOnce();
    }
}
