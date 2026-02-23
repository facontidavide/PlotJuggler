#include "websocket_client_config.h"

WebsocketClientConfig::WebsocketClientConfig() = default;

// =========================
// XML (PlotJuggler layout)
// =========================
void WebsocketClientConfig::xmlSaveState(QDomDocument& doc, QDomElement& plugin_elem) const
{
  QDomElement cfg = doc.createElement("websocket_client");
  plugin_elem.appendChild(cfg);

  cfg.setAttribute("address", address);
  cfg.setAttribute("port", port);

  QDomElement topics_elem = doc.createElement("topics");
  cfg.appendChild(topics_elem);

  for (const auto& topic : topics)
  {
    QDomElement t = doc.createElement("topic");
    t.setAttribute("name", topic);
    topics_elem.appendChild(t);
  }
}

void WebsocketClientConfig::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement cfg = parent_element.firstChildElement("websocket_client");
  if (cfg.isNull())
  {
    return;
  }

  address = cfg.attribute("address", "127.0.0.1");
  port = cfg.attribute("port", "8080").toInt();

  topics.clear();

  QDomElement topics_elem = cfg.firstChildElement("topics");
  for (QDomElement t = topics_elem.firstChildElement("topic"); !t.isNull();
       t = t.nextSiblingElement("topic"))
  {
    QString name = t.attribute("name");
    if (!name.isEmpty())
    {
      topics.push_back(name);
    }
  }
}

// =========================
// QSettings (global defaults)
// =========================
void WebsocketClientConfig::saveToSettings(QSettings& settings, const QString& group) const
{
  settings.setValue(group + "/address", address);
  settings.setValue(group + "/port", port);
  settings.setValue(group + "/topics", topics);
}

void WebsocketClientConfig::loadFromSettings(const QSettings& settings, const QString& group)
{
  address = settings.value(group + "/address", "127.0.0.1").toString();
  port = settings.value(group + "/port", 8080).toInt();
  topics = settings.value(group + "/topics").toStringList();
}
