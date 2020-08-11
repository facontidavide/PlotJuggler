#include "qwsdialog.h"
#include "ui_qwsdialog.h"
#include <boost/make_shared.hpp>
#include <QSettings>
#include <QMessageBox>

QWSDialog::QWSDialog(QWidget* parent) : QDialog(parent), ui(new Ui::QWSDialog)
{
  ui->setupUi(this);

  QSettings settings;

  auto master_ui = settings.value("QNode.master_uri", tr("http://localhost:11311")).toString();
  auto host_ip = settings.value("QNode.host_ip", tr("localhost")).toString();

}

namespace ros
{
namespace master
{
void init(const M_string& remappings);
}
}  // namespace ros

bool QWSDialog::Connect(const std::string& ros_master_uri, const std::string& hostname)
{
  std::map<std::string, std::string> remappings;
  remappings["__master"] = ros_master_uri;
  remappings["__hostname"] = hostname;

  static bool first_time = true;
  if (first_time)
  {
    ros::init(remappings, "PlotJugglerListener", ros::init_options::AnonymousName);
    first_time = false;
  }
  else
  {
    ros::master::init(remappings);
  }

  bool connected = ros::master::check();
  if (!connected)
  {
    QMessageBox msgBox;
    msgBox.setText(QString("Could not connect to the ros master [%1]").arg(QString::fromStdString(ros_master_uri)));
    msgBox.exec();
  }
  return connected;
}

QWSDialog::~QWSDialog()
{
  QSettings settings;
  delete ui;
}

void QWSDialog::on_pushButtonConnect_pressed()
{
  bool connected = false;
  if(ui->lineEditWebsocket->text().toStdString() != ""){
      WSManager& manager = WSManager::get();
      manager.update( ui->lineEditWebsocket->text().toStdString());
      this->close();
  }
  if (connected)
  {
    this->close();
  }
  else
  {
  }
}

void QWSDialog::on_pushButtonCancel_pressed()
{
  this->close();
}

WSManager& WSManager::get()
{
    static WSManager manager;
    return manager;
}

void WSManager::stopWS()
{
    if (ros::isStarted())
    {
        ros::shutdown();  // explicitly needed since we use ros::start();;
        ros::waitForShutdown();
    }
}

WSManager::~WSManager()
{
}

std::string WSManager::getNode()
{
    WSManager& manager = WSManager::get();

    if (!ros::isInitialized() || !ros::master::check())
    {
        bool connected = QWSDialog::Connect("test", "localhost");
        if (!connected)
        {
            // as a fallback strategy, launch the QNodeDialog
            QWSDialog dialog;
            dialog.exec();
        }
    }
    return m_ws_address;
}