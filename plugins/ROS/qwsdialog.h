#ifndef QWSDIALOG_H
#define QWSDIALOG_H

#include <QDialog>
#include <ros/ros.h>
#include "../3rdparty/rosbridgecpp/rosbridge_ws_client.hpp"

namespace Ui
{
class QWSDialog;
}

class QWSDialog : public QDialog
{
  Q_OBJECT

public:
  ~QWSDialog();
  explicit QWSDialog(QWidget* parent = 0);

  static bool Connect(const std::string& ros_master_uri, const std::string& hostname = "localhost");

private slots:
  void on_pushButtonConnect_pressed();

  void on_pushButtonCancel_pressed();

private:
  Ui::QWSDialog* ui;
};

class WSManager
{
private:
    std::shared_ptr<RosbridgeWsClient> m_ws_client;
    WSManager()
    {
    }
    void stopWS();

public:
    void createWS(std::string _address);
    static WSManager& get();
    ~WSManager();
    std::shared_ptr<RosbridgeWsClient> getWS();
};

#endif  // QWSDIALOG_H
