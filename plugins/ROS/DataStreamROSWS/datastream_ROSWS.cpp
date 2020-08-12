#include "datastream_ROSWS.h"
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
#include <ros_type_introspection/utils/shape_shifter.hpp>
#include <ros/transport_hints.h>
#include <qwsdialog.h>

#include "dialog_select_ros_topics.h"
#include "rule_editing.h"
#include "qnodedialog.h"
#include "shape_shifter_factory.hpp"

std::vector<std::pair<QString, QString>> _all_topics;

DataStreamROSWS::DataStreamROSWS()
        : DataStreamer(), _ws(nullptr), _action_saveIntoRosbag(nullptr), _prev_clock_time(0) {
    _running = false;
    _periodic_timer = new QTimer();
//    connect(_periodic_timer, &QTimer::timeout, this, &DataStreamROSWS::timerCallback); // TODO: enable disconnection monitor on ros ws

    loadDefaultSettings();
}

void flatten(std::vector<std::pair<std::string, double>> *output, std::string name, rapidjson::Value obj){

}

std::vector<std::pair<std::string, double>> flatten_json(std::string prefix, rapidjson::Value *obj){
    std::vector<std::pair<std::string, double>> out_arr;
    if(obj->IsObject()){
        for (auto itr = obj->MemberBegin(); itr != obj->MemberEnd(); ++itr)
        {
            flatten_json(prefix + "/" + itr->name.GetString() , &(*obj)[itr->name]);
        }
    }
    else {
        if(obj->IsDouble()){
            std::cout << prefix << ": "<< obj->GetDouble() << std::endl;
        }
    }
    return out_arr;
}

void DataStreamROSWS::topicCallback(std::shared_ptr<WsClient::InMessage> in_message, const std::string &topic_name) {
    if (!_running) {
        return;
    }

    std::string messagebuf = in_message->string();

    rapidjson::Document document;
    if (document.Parse(messagebuf.c_str()).HasParseError()) {
        std::cerr << "advertiseServiceCallback(): Error in parsing service request message: " << messagebuf
                  << std::endl;
        return;
    }

    flatten_json(topic_name, &document["msg"]);

    double msg_time = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    if(document["msg"].HasMember("header")){
        if (document["msg"]["header"]["stamp"]["secs"].GetDouble() != 0) {
            double sec = document["msg"]["header"]["stamp"]["secs"].GetDouble();
            double nsec = document["msg"]["header"]["stamp"]["nsecs"].GetDouble();
            msg_time = sec + (nsec * 1e-9);
        }
    }

    // before pushing, lock the mutex
    std::lock_guard<std::mutex> lock(mutex());

    const std::string prefixed_topic_name = _prefix + topic_name;

    // adding raw serialized msg for future uses.
    // do this before msg_time normalization
    {
        auto plot_pair = dataMap().user_defined.find(prefixed_topic_name);
        if (plot_pair == dataMap().user_defined.end()) {
            plot_pair = dataMap().addUserDefined(prefixed_topic_name);
        }
        PlotDataAny &user_defined_data = plot_pair->second;
        user_defined_data.pushBack(PlotDataAny::Point(msg_time, nonstd::any(std::move(in_message->string()))));
    }

    //------------------------------
    {
        int &index = _msg_index[topic_name];
        index++;
        const std::string key = prefixed_topic_name + ("/_MSG_INDEX_");
        auto index_it = dataMap().numeric.find(key);
        if (index_it == dataMap().numeric.end()) {
            index_it = dataMap().addNumeric(key);
        }
        index_it->second.pushBack(PlotData::Point(msg_time, index));
    }
}

void DataStreamROSWS::timerCallback() {
    if (_running) {
        auto ret = QMessageBox::warning(nullptr, tr("Disconnected!"),
                                        tr("The roscore master cannot is not reachable anymore.\n\n"
                                           "Do you want to try reconnecting to it? \n"),
                                        tr("Stop Streaming"), tr("Try reconnect"), nullptr);
        if (ret == 1) {
            this->shutdown();
            _ws = WSManager::get().getWS();

            if (!_ws) {
                emit connectionClosed();
                return;
            }
            _parser.reset(new CompositeParser(dataMap()));
            subscribe();

            _running = true;
            _periodic_timer->start();
        } else if (ret == 0) {
            this->shutdown();
            emit connectionClosed();
        }
    }
}

void DataStreamROSWS::saveIntoRosbag(const PlotDataMapRef &data) {
    if (data.user_defined.empty()) {
        QMessageBox::warning(nullptr, tr("Warning"), tr("Your buffer is empty. Nothing to save.\n"));
        return;
    }

    QFileDialog saveDialog;
    saveDialog.setAcceptMode(QFileDialog::AcceptSave);
    saveDialog.setDefaultSuffix("bag");
    saveDialog.exec();

    if (saveDialog.result() != QDialog::Accepted || saveDialog.selectedFiles().empty()) {
        return;
    }

    QString fileName = saveDialog.selectedFiles().first();

    if (fileName.size() > 0) {
        rosbag::Bag rosbag(fileName.toStdString(), rosbag::bagmode::Write);

        for (const auto &it : data.user_defined) {
            const std::string &topicname = it.first;
            const auto &plotdata = it.second;

            auto registered_msg_type = RosIntrospectionFactory::get().getShapeShifter(topicname);
            if (!registered_msg_type)
                continue;

            RosIntrospection::ShapeShifter msg;
            msg.morph(registered_msg_type->getMD5Sum(), registered_msg_type->getDataType(),
                      registered_msg_type->getMessageDefinition());

            for (int i = 0; i < plotdata.size(); i++) {
                const auto &point = plotdata.at(i);
                const PlotDataAny::TimeType msg_time = point.x;
                const nonstd::any &type_erased_buffer = point.y;

                if (type_erased_buffer.type() != typeid(std::vector<uint8_t>)) {
                    // can't cast to expected type
                    continue;
                }

                std::vector<uint8_t> raw_buffer = nonstd::any_cast<std::vector<uint8_t>>(type_erased_buffer);
                ros::serialization::IStream stream(raw_buffer.data(), raw_buffer.size());
                msg.read(stream);

                rosbag.write(topicname, ros::Time(msg_time), msg);
            }
        }
        rosbag.close();

        QProcess process;
        QStringList args;
        args << "reindex" << fileName;
        process.start("rosbag", args);
    }
}

void DataStreamROSWS::subscribe() {
    _ws_subscribers.clear();

    QProgressDialog progress_dialog;
    progress_dialog.setLabelText("Collecting ROS topic samples to understand data layout. ");
    progress_dialog.setRange(0, _config.selected_topics.size());
    progress_dialog.setAutoClose(true);
    progress_dialog.setAutoReset(true);

    progress_dialog.show();

    for (int i = 0; i < _config.selected_topics.size(); i++) {


        const std::string topic_name = _config.selected_topics[i].toStdString();
        boost::function<void(std::shared_ptr<WsClient::Connection> /*connection*/,
                             std::shared_ptr<WsClient::InMessage> in_message)> callback;
        callback = [this, topic_name](std::shared_ptr<WsClient::Connection> /*connection*/,
                                      std::shared_ptr<WsClient::InMessage> in_message) -> void {
            this->topicCallback(in_message, topic_name);
        };


        _ws_subscribers.insert({topic_name, RosbridgeWsSubscriber(_ws, topic_name, callback)});

        progress_dialog.setValue(i);
        QApplication::processEvents();
        if (progress_dialog.wasCanceled()) {
            break;
        }
    }
}

void
topicsCallback(std::shared_ptr<WsClient::Connection> /*connection*/, std::shared_ptr<WsClient::InMessage> in_message) {
    std::string messagebuf = in_message->string();
    rapidjson::Document document;
    if (document.Parse(messagebuf.c_str()).HasParseError()) {
        std::cerr << "advertiseServiceCallback(): Error in parsing service request message: " << messagebuf
                  << std::endl;
        return;
    }
    if (document["values"]["topics"].IsArray()) {
        for (int i = 0; i < document["values"]["topics"].Size(); i++) {
            _all_topics.push_back(std::make_pair(QString(document["values"]["topics"][i].GetString()),
                                                 QString(document["values"]["types"][i].GetString())));
        }
    }
}

bool DataStreamROSWS::start(QStringList *selected_datasources) {
    if (!_ws) {
        WSManager &manager = WSManager::get();
        _ws = manager.getWS();
        _ws->addClient("service_advertiser");
        _ws->callService("/rosapi/topics", &topicsCallback);
    }

    if (!_ws) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex());
        dataMap().numeric.clear();
        dataMap().user_defined.clear();
        _parser.reset(new CompositeParser(dataMap()));
    }

    using namespace RosIntrospection;

    QTimer timer;
    timer.setSingleShot(false);
    timer.setInterval(5000);
    timer.start();

    DialogSelectRosTopics dialog(_all_topics, _config);

//    connect(&timer, &QTimer::timeout, [&]() {
//        _all_topics.clear();
//        _ws->callService("/rosapi/topics", &topicsCallback);
//        dialog.updateTopicList(_all_topics);
//    });

    int res = dialog.exec();
    _config = dialog.getResult();
    timer.stop();

    if (res != QDialog::Accepted || _config.selected_topics.empty()) {
        return false;
    }

    saveDefaultSettings();

    if (_config.discard_large_arrays) {
        _parser->setMaxArrayPolicy(DISCARD_LARGE_ARRAYS, _config.max_array_size);
    } else {
        _parser->setMaxArrayPolicy(KEEP_LARGE_ARRAYS, _config.max_array_size);
    }

    //-------------------------
    subscribe();
    _running = true;

    _periodic_timer->setInterval(500);
    _periodic_timer->start();

    return true;
}

bool DataStreamROSWS::isRunning() const {
    return _running;
}

void DataStreamROSWS::shutdown() {
    _periodic_timer->stop();

    _ws.reset();

    _ws_subscribers.clear();
    _running = false;
    _parser.reset();
}

DataStreamROSWS::~DataStreamROSWS() {
    shutdown();
}

bool DataStreamROSWS::xmlSaveState(QDomDocument &doc, QDomElement &plugin_elem) const {
    QDomElement stamp_elem = doc.createElement("use_header_stamp");
    stamp_elem.setAttribute("value", _config.use_header_stamp ? "true" : "false");
    plugin_elem.appendChild(stamp_elem);

    QDomElement rename_elem = doc.createElement("use_renaming_rules");
    rename_elem.setAttribute("value", _config.use_renaming_rules ? "true" : "false");
    plugin_elem.appendChild(rename_elem);

    QDomElement discard_elem = doc.createElement("discard_large_arrays");
    discard_elem.setAttribute("value", _config.discard_large_arrays ? "true" : "false");
    plugin_elem.appendChild(discard_elem);

    QDomElement max_elem = doc.createElement("max_array_size");
    max_elem.setAttribute("value", QString::number(_config.max_array_size));
    plugin_elem.appendChild(max_elem);

    return true;
}

bool DataStreamROSWS::xmlLoadState(const QDomElement &parent_element) {
    QDomElement stamp_elem = parent_element.firstChildElement("use_header_stamp");
    _config.use_header_stamp = (stamp_elem.attribute("value") == "true");

    QDomElement rename_elem = parent_element.firstChildElement("use_renaming_rules");
    _config.use_renaming_rules = (rename_elem.attribute("value") == "true");

    QDomElement discard_elem = parent_element.firstChildElement("discard_large_arrays");
    _config.discard_large_arrays = (discard_elem.attribute("value") == "true");

    QDomElement max_elem = parent_element.firstChildElement("max_array_size");
    _config.max_array_size = max_elem.attribute("value").toInt();

    return true;
}

void DataStreamROSWS::addActionsToParentMenu(QMenu *menu) {
    _action_saveIntoRosbag = new QAction(QString("Save cached value in a rosbag"), menu);
    menu->addAction(_action_saveIntoRosbag);

    connect(_action_saveIntoRosbag, &QAction::triggered, this,
            [this]() { DataStreamROSWS::saveIntoRosbag(dataMap()); });
}

void DataStreamROSWS::saveDefaultSettings() {
    QSettings settings;

    settings.setValue("DataStreamROSWS/default_topics", _config.selected_topics);
    settings.setValue("DataStreamROSWS/use_renaming", _config.use_renaming_rules);
    settings.setValue("DataStreamROSWS/use_header_stamp", _config.use_header_stamp);
    settings.setValue("DataStreamROSWS/max_array_size", (int) _config.max_array_size);
    settings.setValue("DataStreamROSWS/discard_large_arrays", _config.discard_large_arrays);
}

void DataStreamROSWS::loadDefaultSettings() {
    QSettings settings;
    _config.selected_topics = settings.value("DataStreamROSWS/default_topics", false).toStringList();
    _config.use_header_stamp = settings.value("DataStreamROSWS/use_header_stamp", false).toBool();
    _config.use_renaming_rules = settings.value("DataStreamROSWS/use_renaming", true).toBool();
    _config.max_array_size = settings.value("DataStreamROSWS/max_array_size", 100).toInt();
    _config.discard_large_arrays = settings.value("DataStreamROSWS/discard_large_arrays", true).toBool();
}
