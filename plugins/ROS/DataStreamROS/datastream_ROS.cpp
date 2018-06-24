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
#include <QSettings>
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

    _periodic_timer = new QTimer();
    connect( _periodic_timer, &QTimer::timeout,
             this, &DataStreamROS::timerCallback);
}

PlotDataMap& DataStreamROS::getDataMap()
{
    return _plot_data;
}

void DataStreamROS::topicCallback(const topic_tools::ShapeShifter::ConstPtr& msg,
                                  const std::string &topic_name)
{
    if( !_running ||  !_enabled){
        return;
    }

    using namespace RosIntrospection;
    const auto&  md5sum     =  msg->getMD5Sum();
    const auto&  datatype   =  msg->getDataType();
    const auto&  definition =  msg->getMessageDefinition() ;

      // register the message type
    _parser->registerMessageDefinition(topic_name, ROSType(datatype), definition);
    RosIntrospectionFactory::registerMessage(topic_name, md5sum, datatype, definition );

    if( _using_renaming_rules ){
      for (auto it: _rules)
      {
        _parser->registerRenamingRules( it.first, it.second );
      }
    }


    //------------------------------------

    // it is more efficient to recycle this elements
    static std::vector<uint8_t> buffer;
    static FlatMessage flat_container;
    static RenamedValues renamed_value;
    
    buffer.resize( msg->size() );
    
    ros::serialization::OStream stream(buffer.data(), buffer.size());
    msg->write(stream);

    // WORKAROUND. There are some problems related to renaming when the character / is
    // used as prefix. We will remove that here.
    //if( topicname_SS.at(0) == '/' ) topicname_SS = SString( topic_name.data() +1,  topic_name.size()-1 );

    _parser->deserializeIntoFlatContainer( topic_name, absl::Span<uint8_t>(buffer), &flat_container, _max_array_size);
    _parser->applyNameTransform( topic_name, flat_container, &renamed_value );

    double msg_time = ros::Time::now().toSec();

    if(_use_header_timestamp)
    {
        const auto header_stamp = FlatContainedContainHeaderStamp(renamed_value);
        if(header_stamp){
            const double time = header_stamp.value();
            if( time > 0 )
            {
              msg_time = time;
            }
        }
    }

    // adding raw serialized msg for future uses.
    // do this before msg_time normalization
    {
        const std::string key = _prefix + topic_name;
        auto plot_pair = _plot_data.user_defined.find( key );
        if( plot_pair == _plot_data.user_defined.end() )
        {
            PlotDataAnyPtr temp(new PlotDataAny(key.c_str()));
            auto res = _plot_data.user_defined.insert( std::make_pair( key, temp ) );
            plot_pair = res.first;
        }
        PlotDataAnyPtr& user_defined_data = plot_pair->second;
        user_defined_data->pushBack( PlotDataAny::Point(msg_time, nonstd::any(std::move(buffer)) ));
    }

    for(auto& it: renamed_value )
    {
        const std::string key = ( _prefix + it.first  );
        const double value = it.second.convert<double>();
        auto plot_it = _plot_data.numeric.find(key);
        if( plot_it == _plot_data.numeric.end())
        {
            auto res =   _plot_data.numeric.insert(
                        std::make_pair( key, std::make_shared<PlotData>(key.c_str()) ));
            plot_it = res.first;
        }
        plot_it->second->pushBackAsynchronously( PlotData::Point(msg_time, value));
    }

    //------------------------------
    {
        int& index = _msg_index[topic_name];
        index++;
        const std::string key = _prefix + topic_name + ("/_MSG_INDEX_") ;
        auto index_it = _plot_data.numeric.find(key);
        if( index_it == _plot_data.numeric.end())
        {
            auto res = _plot_data.numeric.insert(
                        std::make_pair( key, std::make_shared<PlotData>(key.c_str()) ));
            index_it = res.first;
        }
        index_it->second->pushBackAsynchronously( PlotData::Point(msg_time, index) );
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

void DataStreamROS::timerCallback()
{
    if( _running && _enabled && ros::master::check() == false
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
        _roscore_disconnection_already_notified = ( ret == 2);
        if( ret == 1)
        {
            this->shutdown();
            _node =  RosManager::getNode();

            if( !_node ){
                emit connectionClosed();
                return;
            }
            subscribe();

            _running = true;
            _enabled = true;
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


void DataStreamROS::subscribe()
{
    _subscribers.clear();

    for (int i=0; i< _default_topic_names.size(); i++ )
    {
        boost::function<void(const topic_tools::ShapeShifter::ConstPtr&) > callback;
        const std::string topic_name = _default_topic_names[i].toStdString();
        callback = [this, topic_name](const topic_tools::ShapeShifter::ConstPtr& msg) -> void
        {
            this->topicCallback(msg, topic_name) ;
        };
        _subscribers.insert( {topic_name, _node->subscribe( topic_name, 0,  callback)}  );
    }
}

bool DataStreamROS::start()
{
    _parser.reset( new RosIntrospection::Parser );
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

    QSettings settings( "IcarusTechnology", "PlotJuggler");

    if( _default_topic_names.empty())
    {
        // if _default_topic_names is empty (xmlLoad didn't work) use QSettings.
        QVariant def = settings.value("DataStreamROS/default_topics");
        if( !def.isNull() && def.isValid())
        {
            _default_topic_names = def.toStringList();
        }
    }

    QTimer timer;
    timer.setSingleShot(false);
    timer.setInterval( 1000);
    timer.start();

    DialogSelectRosTopics dialog(all_topics, _default_topic_names );

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

    _using_renaming_rules = dialog.checkBoxUseRenamingRules()->isChecked();

    if( res != QDialog::Accepted || dialog.getSelectedItems().empty() )
    {
        return false;
    }

     _default_topic_names = dialog.getSelectedItems();

    settings.setValue("DataStreamROS/default_topics", _default_topic_names);

    // load the rules
    if( dialog.checkBoxUseRenamingRules()->isChecked() ){
        _rules = RuleEditing::getRenamingRules();
    }

    _prefix = dialog.prefix().toStdString();

    _use_header_timestamp = dialog.checkBoxUseHeaderStamp()->isChecked();
    _max_array_size = dialog.maxArraySize();
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

void DataStreamROS::enableStreaming(bool enable) { _enabled = enable; }

bool DataStreamROS::isStreamingRunning() const { return _running; }


void DataStreamROS::shutdown()
{
    _periodic_timer->stop();
    _spinner->stop();
    for(auto& it: _subscribers)
    {
        it.second.shutdown();
    }
    _subscribers.clear();
    _running = false;
    _enabled = false;
    _node.reset();
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

QDomElement DataStreamROS::xmlSaveState(QDomDocument &doc) const
{
    QString topics_list = _default_topic_names.join(";");
    QDomElement list_elem = doc.createElement("selected_topics");
    list_elem.setAttribute("list", topics_list );
    return list_elem;
}

bool DataStreamROS::xmlLoadState(QDomElement &parent_element)
{
    QDomElement list_elem = parent_element.firstChildElement( "selected_topics" );
    if( !list_elem.isNull()    )
    {
        if( list_elem.hasAttribute("list") )
        {
            QString topics_list = list_elem.attribute("list");
            _default_topic_names = topics_list.split(";", QString::SkipEmptyParts);
            return true;
        }
    }
    return false;
}

