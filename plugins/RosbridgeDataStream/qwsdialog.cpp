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
      manager.createWS( ui->lineEditWebsocket->text().toStdString());
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

}

WSManager::~WSManager()
{
}

void WSManager::createWS(std::string _address) {
    m_ws_client = std::make_shared<RosbridgeWsClient>(_address);
}

std::shared_ptr<RosbridgeWsClient> WSManager::getWS()
{
    // as a fallback strategy, launch the QNodeDialog
    QWSDialog dialog;
    dialog.exec();

    return m_ws_client;
}