#include "statepublisher_rostopic.h"
#include "PlotJuggler/any.hpp"
#include "qnodedialog.h"

#include "ros_type_introspection/ros_introspection.hpp"
#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <rosbag/bag.h>
#include <std_msgs/Header.h>
#include <unordered_map>
#include <rosgraph_msgs/Clock.h>
#include <QMessageBox>
#include "publisher_select_dialog.h"
#include "QtCore/qjsondocument.h"
#include "QtCore/qjsonobject.h"
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

TopicPublisherROS::TopicPublisherROS() : _enabled(false), _node(nullptr), _publish_clock(true) {
    QSettings settings;
    _publish_clock = settings.value("TopicPublisherROS/publish_clock", true).toBool();
}

TopicPublisherROS::~TopicPublisherROS() {
    _enabled = false;
}

void TopicPublisherROS::setParentMenu(QMenu *menu, QAction *action) {
    StatePublisher::setParentMenu(menu, action);

    _enable_self_action = menu->actions().back();

    _select_topics_to_publish = new QAction(QString("Select topics to be published"), _menu);
    _menu->addAction(_select_topics_to_publish);
    connect(_select_topics_to_publish, &QAction::triggered, this, &TopicPublisherROS::filterDialog);
}

void TopicPublisherROS::setEnabled(bool to_enable) {
    if (!_node && to_enable) {
        _node = RosManager::getNode();
    }
    _enabled = (to_enable && _node);

    if (_enabled) {
        filterDialog(true);

        if (!_tf_broadcaster) {
            _tf_broadcaster = std::make_unique<tf2_ros::TransformBroadcaster>();
        }
        if (!_tf_static_broadcaster) {
            _tf_static_broadcaster = std::make_unique<tf2_ros::StaticTransformBroadcaster>();
        }

        _previous_play_index = std::numeric_limits<int>::max();

        if (_publish_clock) {
            _clock_publisher = _node->advertise<rosgraph_msgs::Clock>("/clock", 10, true);
        } else {
            _clock_publisher.shutdown();
        }
    } else {
        _node.reset();
        _publishers.clear();
        _clock_publisher.shutdown();

        _tf_broadcaster.reset();
        _tf_static_broadcaster.reset();
    }

    StatePublisher::setEnabled(_enabled);
}

void TopicPublisherROS::filterDialog(bool autoconfirm) {
    auto all_topics = RosIntrospectionFactory::get().getTopicList();

    if (all_topics.empty()) {
        return;
    }

    PublisherSelectDialog *dialog = new PublisherSelectDialog();

    std::map<std::string, QCheckBox *> checkbox;

    for (const auto &topic : all_topics) {
        auto cb = new QCheckBox(dialog);
        auto filter_it = _topics_to_publish.find(*topic);
        if (filter_it == _topics_to_publish.end()) {
            cb->setChecked(true);
        } else {
            cb->setChecked(filter_it->second);
        }
        cb->setFocusPolicy(Qt::NoFocus);
        dialog->ui()->formLayout->addRow(new QLabel(QString::fromStdString(*topic)), cb);
        checkbox.insert(std::make_pair(*topic, cb));
        connect(dialog->ui()->pushButtonSelect, &QPushButton::pressed, [cb]() { cb->setChecked(true); });
        connect(dialog->ui()->pushButtonDeselect, &QPushButton::pressed, [cb]() { cb->setChecked(false); });
    }

    if (!autoconfirm) {
        dialog->exec();
    }

    if (autoconfirm || dialog->result() == QDialog::Accepted) {
        _topics_to_publish.clear();
        for (const auto &it : checkbox) {
            _topics_to_publish.insert({it.first, it.second->isChecked()});
        }

        // remove already created publisher if not needed anymore
        for (auto it = _publishers.begin(); it != _publishers.end(); /* no increment */) {
            const std::string &topic_name = it->first;
            if (!toPublish(topic_name)) {
                it = _publishers.erase(it);
            } else {
                it++;
            }
        }

        _publish_clock = dialog->ui()->radioButtonClock->isChecked();

        if (_enabled && _publish_clock) {
            _clock_publisher = _node->advertise<rosgraph_msgs::Clock>("/clock", 10, true);
        } else {
            _clock_publisher.shutdown();
        }

        dialog->deleteLater();

        QSettings settings;
        settings.setValue("TopicPublisherROS/publish_clock", _publish_clock);
    }
}

void TopicPublisherROS::broadcastTF(double current_time) {
    using StringPair = std::pair<std::string, std::string>;

    std::map<StringPair, geometry_msgs::TransformStamped> transforms;
    std::map<StringPair, geometry_msgs::TransformStamped> static_transforms;

    for (const auto &data_it : _datamap->user_defined) {
        const std::string &topic_name = data_it.first;
        const PlotDataAny &plot_any = data_it.second;

        if (!toPublish(topic_name)) {
            continue;  // Not selected
        }

        if (topic_name != "/tf_static" && topic_name != "/tf") {
            continue;
        }

        const PlotDataAny *tf_data = &plot_any;
        int last_index = tf_data->getIndexFromX(current_time);
        if (last_index < 0) {
            continue;
        }

        auto transforms_ptr = (topic_name == "/tf_static") ? &static_transforms : &transforms;

        std::vector<uint8_t> raw_buffer;
        // 2 seconds in the past (to be configurable in the future)
        int initial_index = tf_data->getIndexFromX(current_time - 2.0);

        if (_previous_play_index < last_index && _previous_play_index > initial_index) {
            initial_index = _previous_play_index;
        }

        for (size_t index = std::max(0, initial_index); index <= last_index; index++) {
            const nonstd::any &any_value = tf_data->at(index).y;

            const bool isRosbagMessage = any_value.type() == typeid(rosbag::MessageInstance);

            if (!isRosbagMessage) {
                continue;
            }

            const auto &msg_instance = nonstd::any_cast<rosbag::MessageInstance>(any_value);

            raw_buffer.resize(msg_instance.size());
            ros::serialization::OStream ostream(raw_buffer.data(), raw_buffer.size());
            msg_instance.write(ostream);

            tf2_msgs::TFMessage tf_msg;
            ros::serialization::IStream istream(raw_buffer.data(), raw_buffer.size());
            ros::serialization::deserialize(istream, tf_msg);

            for (const auto &stamped_transform : tf_msg.transforms) {
                const auto &parent_id = stamped_transform.header.frame_id;
                const auto &child_id = stamped_transform.child_frame_id;
                StringPair trans_id = std::make_pair(parent_id, child_id);
                auto it = transforms_ptr->find(trans_id);
                if (it == transforms_ptr->end()) {
                    transforms_ptr->insert({trans_id, stamped_transform});
                } else if (it->second.header.stamp <= stamped_transform.header.stamp) {
                    it->second = stamped_transform;
                }
            }
        }
        // ready to broadacast
        std::vector<geometry_msgs::TransformStamped> transforms_vector;
        transforms_vector.reserve(transforms_ptr->size());

        const auto now = ros::Time::now();
        for (auto &trans : *transforms_ptr) {
            if (!_publish_clock) {
                trans.second.header.stamp = now;
            }
            transforms_vector.emplace_back(std::move(trans.second));
        }
        if (transforms_vector.size() > 0) {
            if (transforms_ptr == &transforms) {
                _tf_broadcaster->sendTransform(transforms_vector);
            } else {
                _tf_static_broadcaster->sendTransform(transforms_vector);
            }
        }
    }
}

bool TopicPublisherROS::toPublish(const std::string &topic_name) {
    auto it = _topics_to_publish.find(topic_name);
    if (it == _topics_to_publish.end()) {
        return false;
    } else {
        return it->second;
    }
}
template<class T>
void addLeaf(json *obj, const std::string& path, T value){
    auto tail = path.substr(1, path.size() - 1); // remove first /
    if(tail.find('/') != std::string::npos){
        auto current = tail.substr(0, tail.find('/'));
        tail.erase(0, current.size());
        addLeaf(&(*obj)[current.c_str()], tail, value);
    } else {
        if(std::is_same<T, nonstd::span<uint8_t>>::value){ // check if type is blob
            (*obj)[tail.c_str()] = value;
        }
        else if(tail.find('.') != std::string::npos){
            auto name = tail.substr(0, tail.find('.'));
            auto pos = std::stoi(tail.substr(tail.find('.') + 1, tail.size() - 1),nullptr,0);
            (*obj)[name.c_str()][pos] = value;
        }
        else {
            (*obj)[tail.c_str()] = value;
        }
    }
}

json messageToJson(const rosbag::MessageInstance &msg_instance, bool publish_clock){
    using namespace RosIntrospection;

    const auto &topic_name = msg_instance.getTopic();

    std::vector<uint8_t> raw_buffer;
    raw_buffer.resize(msg_instance.size());
    ros::serialization::OStream ostream(raw_buffer.data(), raw_buffer.size());
    msg_instance.write(ostream);

    Parser parser;
    const std::string&  datatype   =  msg_instance.getDataType();
    const std::string&  definition =  msg_instance.getMessageDefinition();

    // don't worry if you do this more than once
    parser.registerMessageDefinition( topic_name,
                                      RosIntrospection::ROSType(datatype),
                                      definition );

    FlatMessage   flat_container;
    RenamedValues renamed_value;
    parser.deserializeIntoFlatContainer( topic_name,
                                         Span<uint8_t>(raw_buffer),
                                         &flat_container,
                                         100);

    parser.applyNameTransform( topic_name, flat_container, &renamed_value );

    json msg;
    msg["op"] = "publish";
    msg["topic"] = topic_name;

    // Print the content of the message
    for (auto it: renamed_value)
    {
        const std::string& key = it.first;
        const Variant& value   = it.second;
        if(!publish_clock){
            if(key.find("header") != std::string::npos){
                auto header = ros::Time::now().toSec();
                if(key.find("stamp") != std::string::npos) {
                    addLeaf(&msg["msg"], key, header);
                }
                else {
                    addLeaf(&msg["msg"], key, value.convert<double>());
                }
            }
            else {
                addLeaf(&msg["msg"], key, value.convert<double>());
            }
        } else {
            addLeaf(&msg["msg"], key, value.convert<double>());
        }
    }
    for (auto it: flat_container.name)
    {
        const std::string& key    = it.first.toStdString();
        const std::string& value  = it.second;
        addLeaf(&msg["msg"], key, value);
    }
    for (auto it: flat_container.blob)
    {
        const std::string& key    = it.first.toStdString();
        auto value  = it.second;
        addLeaf(&msg["msg"], key, value);
    }

    return msg;
}

void TopicPublisherROS::publishAnyMsg(const rosbag::MessageInstance &msg_instance) {
    auto msg = messageToJson(msg_instance, _publish_clock);
    std::cout << msg << std::endl;
    using namespace RosIntrospection;

    const auto &topic_name = msg_instance.getTopic();

    RosIntrospection::ShapeShifter *shapeshifted = RosIntrospectionFactory::get().getShapeShifter(topic_name);

    if (!shapeshifted) {
        return;  // Not registered, just skip
    }

    std::vector<uint8_t> raw_buffer;
    raw_buffer.resize(msg_instance.size());
    ros::serialization::OStream ostream(raw_buffer.data(), raw_buffer.size());
    msg_instance.write(ostream);

    if (!_publish_clock) {
        const ROSMessageInfo *msg_info = RosIntrospectionFactory::parser().getMessageInfo(topic_name);
        if (msg_info && msg_info->message_tree.croot()->children().size() >= 1) {
            const auto &first_field = msg_info->message_tree.croot()->child(0)->value();
            if (first_field->type().baseName() == "std_msgs/Header") {
                std_msgs::Header msg;
                ros::serialization::IStream is(raw_buffer.data(), raw_buffer.size());
                ros::serialization::deserialize(is, msg);
                msg.stamp = ros::Time::now();
                ros::serialization::OStream os(raw_buffer.data(), raw_buffer.size());
                ros::serialization::serialize(os, msg);
            }
        }
    }

    ros::serialization::IStream istream(raw_buffer.data(), raw_buffer.size());
    shapeshifted->read(istream);

    auto publisher_it = _publishers.find(topic_name);
    if (publisher_it == _publishers.end()) {
        auto res = _publishers.insert({topic_name, shapeshifted->advertise(*_node, topic_name, 10, true)});
        publisher_it = res.first;
    }

    const ros::Publisher &publisher = publisher_it->second;
    publisher.publish(*shapeshifted);
}

void TopicPublisherROS::updateState(double current_time) {
    if (!_enabled || !_node)
        return;

    if (!ros::master::check()) {
        QMessageBox::warning(nullptr, tr("Disconnected!"),
                             "The roscore master cannot be detected.\n"
                             "The publisher will be disabled.");
        _enable_self_action->setChecked(false);
        return;
    }

    //-----------------------------------------------
    broadcastTF(current_time);
    //-----------------------------------------------

    auto data_it = _datamap->user_defined.find("__consecutive_message_instances__");
    if (data_it != _datamap->user_defined.end()) {
        const PlotDataAny &continuous_msgs = data_it->second;
        _previous_play_index = continuous_msgs.getIndexFromX(current_time);
        // qDebug() << QString("u: %1").arg( current_index ).arg(current_time, 0, 'f', 4 );
    }

    for (const auto &data_it : _datamap->user_defined) {
        const std::string &topic_name = data_it.first;
        const PlotDataAny &plot_any = data_it.second;
        if (!toPublish(topic_name)) {
            continue;  // Not selected
        }

        const RosIntrospection::ShapeShifter *shapeshifter = RosIntrospectionFactory::get().getShapeShifter(topic_name);

        if (shapeshifter->getDataType() == "tf/tfMessage" || shapeshifter->getDataType() == "tf2_msgs/TFMessage") {
            continue;
        }

        int last_index = plot_any.getIndexFromX(current_time);
        if (last_index < 0) {
            continue;
        }

        const auto &any_value = plot_any.at(last_index).y;

        if (any_value.type() == typeid(rosbag::MessageInstance)) {
            const auto &msg_instance = nonstd::any_cast<rosbag::MessageInstance>(any_value);
            publishAnyMsg(msg_instance);
        }
    }

    if (_publish_clock) {
        rosgraph_msgs::Clock clock;
        try {
            clock.clock.fromSec(current_time);
            _clock_publisher.publish(clock);
        }
        catch (...) {
            qDebug() << "error: " << current_time;
        }
    }
}

void TopicPublisherROS::play(double current_time) {
    if (!_enabled || !_node)
        return;

    if (!ros::master::check()) {
        QMessageBox::warning(nullptr, tr("Disconnected!"),
                             "The roscore master cannot be detected.\n"
                             "The publisher will be disabled.");
        _enable_self_action->setChecked(false);
        return;
    }

    auto data_it = _datamap->user_defined.find("__consecutive_message_instances__");
    if (data_it == _datamap->user_defined.end()) {
        return;
    }
    const PlotDataAny &continuous_msgs = data_it->second;
    int current_index = continuous_msgs.getIndexFromX(current_time);

    if (_previous_play_index > current_index) {
        _previous_play_index = current_index;
        updateState(current_time);
        return;
    } else {
        const PlotDataAny &consecutive_msg = data_it->second;
        for (int index = _previous_play_index + 1; index <= current_index; index++) {
            const auto &any_value = consecutive_msg.at(index).y;
            if (any_value.type() == typeid(rosbag::MessageInstance)) {
                const auto &msg_instance = nonstd::any_cast<rosbag::MessageInstance>(any_value);

                if (!toPublish(msg_instance.getTopic())) {
                    continue;  // Not selected
                }

                // qDebug() << QString("p: %1").arg( index );
                publishAnyMsg(msg_instance);

                if (_publish_clock) {
                    rosgraph_msgs::Clock clock;
                    clock.clock = msg_instance.getTime();
                    _clock_publisher.publish(clock);
                }
            }
        }
        _previous_play_index = current_index;
    }
}
