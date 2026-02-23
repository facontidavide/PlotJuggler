#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QStringList>
#include <QTreeWidget>

#include "websocket_client_config.h"

struct TopicInfo
{
  QString name;
  QString type;
  QString schema_name;
  QString schema_encoding;
  QString schema_definition;
};

namespace Ui
{
class WebSocketDialog;
}

class WebsocketDialog : public QDialog
{
public:
  explicit WebsocketDialog(const WebsocketClientConfig& config);
  ~WebsocketDialog();

  // Address / port
  QString address() const;

  int port(bool* ok) const;

  // Topic list management
  void setTopics(const QJsonArray& topics, const QStringList& preselectNames);

  bool hasSelection() const;

  QStringList selectedTopicNames() const;

  std::vector<TopicInfo> selectedTopics() const;

  void clearTopics();

  // OK button
  void setOkButton(const QString& text, bool enabled);

  // Signal access for external connections
  QDialogButtonBox* buttonBox() const;

  QTreeWidget* topicsWidget() const;

private:
  void applyFilter(const QString& filter);
  Ui::WebSocketDialog* ui;
};
