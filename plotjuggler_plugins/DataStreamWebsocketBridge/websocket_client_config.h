#ifndef WEBSOCKET_CLIENT_CONFIG_H
#define WEBSOCKET_CLIENT_CONFIG_H

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QDomDocument>

class WebsocketClientConfig
{
public:
  // =========================
  // Persisted fields
  // =========================
  QString address = "127.0.0.1";
  int port = 8080;
  QStringList topics;

  WebsocketClientConfig();

  // =========================
  // XML (PlotJuggler layout)
  // =========================
  void xmlSaveState(QDomDocument& doc, QDomElement& plugin_elem) const;
  void xmlLoadState(const QDomElement& parent_element);

  // =========================
  // QSettings (global defaults)
  // =========================
  void saveToSettings(QSettings& settings, const QString& group) const;
  void loadFromSettings(const QSettings& settings, const QString& group);
};

#endif  // WEBSOCKET_CLIENT_CONFIG_H
