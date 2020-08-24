#include "datastream_ROSWS.h"
#include <QFile>
#include <QMessageBox>
#include <mutex>
#include <chrono>
#include <QProgressDialog>
#include <QtGlobal>
#include <QApplication>
#include <QSettings>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <qwsdialog.h>

#include "dialog_select_ros_topics.h"

DataStreamROSWS::DataStreamROSWS() : DataStreamer(), _fetched_topics(false), _prev_clock_time(0)
{
  _running = false;
  loadDefaultSettings();
}

 void flatten_json(std::vector<std::pair<std::string, double>> *output, std::string prefix, QJsonValueRef obj){
    if(obj.isObject()){
        foreach(const QString& key, obj.toObject().keys()) {
          flatten_json(output, prefix + "/" + key.toStdString() , obj.toObject()[key]);
        }
    }
    else {
        if(obj.isDouble()){
            output->push_back({prefix, obj.toDouble()});
        }
    }
}

QString generateSubscribeTopicMessage(const std::string& topic_name)
{
  QJsonObject topic_request;
  topic_request["op"] = "subscribe";
  topic_request["topic"] = topic_name.c_str();
  QJsonDocument doc(topic_request);
  QString strJson(doc.toJson(QJsonDocument::Compact));
  return strJson;
}

void DataStreamROSWS::subscribe()
{
  QProgressDialog progress_dialog;
  progress_dialog.setLabelText("Collecting ROS topic samples to understand data layout. ");
  progress_dialog.setRange(0, _config.selected_topics.size());
  progress_dialog.setAutoClose(true);
  progress_dialog.setAutoReset(true);

  progress_dialog.show();

  for (int i = 0; i < _config.selected_topics.size(); i++)
  {
    const std::string topic_name = _config.selected_topics[i].toStdString();

    _ws.sendTextMessage(generateSubscribeTopicMessage(topic_name));

    progress_dialog.setValue(i);
    QApplication::processEvents();
    if (progress_dialog.wasCanceled())
    {
      break;
    }
  }
}

QString generateGetTopicsListMessage()
{
  QJsonObject topics_request;
  topics_request["op"] = "call_service";
  topics_request["service"] = "/rosapi/topics";
  QJsonDocument doc(topics_request);
  QString strJson(doc.toJson(QJsonDocument::Compact));
  return strJson;
}

void DataStreamROSWS::onConnected()
{
  qDebug() << "on connection";
  _ws.sendTextMessage(generateGetTopicsListMessage());
}

void DataStreamROSWS::handleWsMessage(const QString& message)
{
  QJsonDocument jsonResponse = QJsonDocument::fromJson(message.toUtf8());
  QJsonObject jsonObject = jsonResponse.object();

  if (jsonObject["service"].toString() == "/rosapi/topics")
  {
    auto topicsArr = jsonObject["values"].toObject()["topics"].toArray();
    auto typesArr = jsonObject["values"].toObject()["types"].toArray();

    for (int i = 0; i < topicsArr.size(); i++)
    {
      _all_topics.emplace_back(topicsArr[i].toString(), typesArr[i].toString());
    }

    if (!_all_topics.empty())
    {
      _fetched_topics = true;
    }
  }
  else if(jsonObject["op"].toString() == "publish")
  {
    double msg_time =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    if (!jsonObject["msg"].toObject()["header"].isNull())
    {
      auto header = jsonObject["msg"].toObject()["header"].toObject();
      if (header["stamp"].toObject()["secs"].toDouble() != 0)
      {
        double sec = header["stamp"].toObject()["secs"].toDouble();
        double nsec = header["stamp"].toObject()["nsecs"].toDouble();
        msg_time = sec + (nsec * 1e-9);
      }
    }

    auto topic_name = jsonObject["topic"].toString().toStdString();

    std::vector<std::pair<std::string, double>> key_value_arr;
    flatten_json(&key_value_arr, topic_name, jsonObject["msg"]);

    for( auto &data : key_value_arr){
        auto index_it = dataMap().numeric.find(data.first);
        if (index_it == dataMap().numeric.end()) {
            index_it = dataMap().addNumeric(data.first);
        }
        index_it->second.pushBack(PlotData::Point(msg_time, data.second));
    }

    const std::string prefixed_topic_name = _prefix + topic_name;

    // adding raw serialized msg for future uses.
    // do this before msg_time normalization
    {
        auto plot_pair = dataMap().user_defined.find(prefixed_topic_name);
        if (plot_pair == dataMap().user_defined.end()) {
            plot_pair = dataMap().addUserDefined(prefixed_topic_name);
        }
        PlotDataAny &user_defined_data = plot_pair->second;
        user_defined_data.pushBack(PlotDataAny::Point(msg_time, nonstd::any(std::move(message))));
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
}

void DataStreamROSWS::onDisconnected()
{
  qDebug() << "on disconnected";
  if (_running)
  {
    auto ret = QMessageBox::warning(nullptr, tr("Disconnected!"),
                                    tr("The rosbridge server cannot is not reachable anymore.\n\n"
                                       "Do you want to try reconnecting to it? \n"),
                                    tr("Stop Streaming"), tr("Try reconnect"), nullptr);
    if (ret == 1)
    {
      this->shutdown();
      if (!_ws.isValid())
      {
        emit connectionClosed();
        return;
      }
      subscribe();

      _running = true;
    }
    else if (ret == 0)
    {
      this->shutdown();
      emit connectionClosed();
    }
  }
}

bool DataStreamROSWS::start(QStringList* selected_datasources)
{
  if (!_ws.isValid())
  {
    QWSDialog dialog;
    dialog.exec();
    auto url = dialog.get_url();

    connect(&_ws, &QWebSocket::connected, this, &DataStreamROSWS::onConnected);
    connect(&_ws, &QWebSocket::disconnected, this, &DataStreamROSWS::onDisconnected);
    connect(&_ws, &QWebSocket::textMessageReceived, this, &DataStreamROSWS::handleWsMessage);
    _ws.open(QUrl(url.c_str()));
  }

  {
    std::lock_guard<std::mutex> lock(mutex());
    dataMap().numeric.clear();
    dataMap().user_defined.clear();
  }

  DialogSelectRosTopics dialog(_all_topics, _config);
  QTimer topics_timer;
  topics_timer.setSingleShot(false);
  topics_timer.setInterval(1000);
  topics_timer.start();

  connect(&topics_timer, &QTimer::timeout, [&]() {
    if (_fetched_topics)
    {
      dialog.updateTopicList(_all_topics);
    }
    else
    {
      _all_topics.clear();
    }
  });

  int res = dialog.exec();
  _config = dialog.getResult();
  topics_timer.stop();

  if (res != QDialog::Accepted || _config.selected_topics.empty())
  {
    return false;
  }

  saveDefaultSettings();

  subscribe();

  _running = true;
  return true;
}

bool DataStreamROSWS::isRunning() const
{
  return _running;
}

void DataStreamROSWS::shutdown()
{
  _ws.close();
  _running = false;
}

DataStreamROSWS::~DataStreamROSWS()
{
  shutdown();
}

bool DataStreamROSWS::xmlSaveState(QDomDocument& doc, QDomElement& plugin_elem) const
{
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

bool DataStreamROSWS::xmlLoadState(const QDomElement& parent_element)
{
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

// void DataStreamROSWS::addActionsToParentMenu(QMenu *menu) {
//    _action_saveIntoRosbag = new QAction(QString("Save cached value in a rosbag"), menu);
//    menu->addAction(_action_saveIntoRosbag);

//    connect(_action_saveIntoRosbag, &QAction::triggered, this,
//            [this]() { DataStreamROSWS::saveIntoRosbag(dataMap()); });
//}

void DataStreamROSWS::saveDefaultSettings()
{
  QSettings settings;

  settings.setValue("DataStreamROSWS/default_topics", _config.selected_topics);
  settings.setValue("DataStreamROSWS/use_renaming", _config.use_renaming_rules);
  settings.setValue("DataStreamROSWS/use_header_stamp", _config.use_header_stamp);
  settings.setValue("DataStreamROSWS/max_array_size", (int)_config.max_array_size);
  settings.setValue("DataStreamROSWS/discard_large_arrays", _config.discard_large_arrays);
}

void DataStreamROSWS::loadDefaultSettings()
{
  QSettings settings;
  _config.selected_topics = settings.value("DataStreamROSWS/default_topics", false).toStringList();
  _config.use_header_stamp = settings.value("DataStreamROSWS/use_header_stamp", false).toBool();
  _config.use_renaming_rules = settings.value("DataStreamROSWS/use_renaming", true).toBool();
  _config.max_array_size = settings.value("DataStreamROSWS/max_array_size", 100).toInt();
  _config.discard_large_arrays = settings.value("DataStreamROSWS/discard_large_arrays", true).toBool();
}
