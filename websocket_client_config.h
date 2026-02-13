#ifndef WEBSOCKET_CLIENT_CONFIG_H
#define WEBSOCKET_CLIENT_CONFIG_H

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QDomDocument>

class websocket_client_config
{
public:
  // =========================
  // Persisted fields
  // =========================
  QString address = "127.0.0.1";
  int port = 8080;
  QStringList topics;

  websocket_client_config() = default;

  // =========================
  // XML (PlotJuggler layout)
  // =========================
  void xmlSaveState(QDomDocument& doc, QDomElement& plugin_elem) const
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

  void xmlLoadState(const QDomElement& parent_element)
  {
    QDomElement cfg = parent_element.firstChildElement("websocket_client");
    if (cfg.isNull())
      return;

    address = cfg.attribute("address", "127.0.0.1");
    port = cfg.attribute("port", "8080").toInt();

    topics.clear();

    QDomElement topics_elem = cfg.firstChildElement("topics");
    for (QDomElement t = topics_elem.firstChildElement("topic");
         !t.isNull();
         t = t.nextSiblingElement("topic"))
    {
      QString name = t.attribute("name");
      if (!name.isEmpty())
        topics.push_back(name);
    }
  }

  // =========================
  // QSettings (global defaults)
  // =========================
  void saveToSettings(QSettings& settings, const QString& group) const
  {
    settings.setValue(group + "/address", address);
    settings.setValue(group + "/port", port);
    settings.setValue(group + "/topics", topics);
  }

  void loadFromSettings(const QSettings& settings, const QString& group)
  {
    address = settings.value(group + "/address", "127.0.0.1").toString();
    port    = settings.value(group + "/port", 8080).toInt();
    topics  = settings.value(group + "/topics").toStringList();
  }
};

#endif  // WEBSOCKET_CLIENT_CONFIG_H
