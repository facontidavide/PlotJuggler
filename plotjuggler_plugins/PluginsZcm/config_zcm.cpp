#include "config_zcm.h"
#include "ui_config_zcm.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QFileInfo>

ConfigZCM::ConfigZCM(QString prefix, QWidget* parent)
  : QWidget(parent), ui(new Ui::ConfigZCM), _prefix(prefix)
{
  ui->setupUi(this);

  QSettings settings;
  auto libs = settings.value(_prefix + "::manual_libs", {}).toStringList();
  for (auto const& lib : libs)
  {
    ui->listWidgetLibs->addItem(lib);
  }

  bool is_manual = settings.value(_prefix + "::is_manual", false).toBool();
  ui->radioManualLibrary->setChecked(is_manual);
}

ConfigZCM::~ConfigZCM()
{
  QSettings settings;

  QStringList libs;
  for (int row = 0; row < ui->listWidgetLibs->count(); row++)
  {
    libs << ui->listWidgetLibs->item(row)->text();
  }
  settings.setValue(_prefix + "::manual_libs", libs);
  settings.setValue(_prefix + "::is_manual", ui->radioManualLibrary->isChecked());
  delete ui;
}

QString ConfigZCM::getLibraries() const
{
  if (ui->radioEnvironmentLibrary->isChecked())
  {
    return getenv("PJ_ZCMTYPES_LIB_PATH");
  }
  else
  {
    QStringList libs;
    for (int row = 0; row < ui->listWidgetLibs->count(); row++)
    {
      libs << ui->listWidgetLibs->item(row)->text();
    }
    return libs.join(":");
  }
}

void ConfigZCM::on_radioEnvironmentLibrary_toggled(bool checked)
{
  if (checked)
  {
    ui->radioManualLibrary->setChecked(false);
  }
}

void ConfigZCM::on_radioManualLibrary_toggled(bool checked)
{
  if (checked)
  {
    ui->radioEnvironmentLibrary->setChecked(false);
  }
  ui->manualWidget->setEnabled(checked);
}

void ConfigZCM::on_pushButtonAdd_clicked()
{
  QSettings settings;
  auto dir = settings.value(_prefix + "::load_dir", QDir::currentPath()).toString();
  auto files = QFileDialog::getOpenFileNames(this, "Zcm libraries", dir, "*.so *.dll");
  for (auto filename : files)
  {
    if (ui->listWidgetLibs->findItems(filename, Qt::MatchExactly).empty())
    {
      ui->listWidgetLibs->addItem(filename);
    }
  }
  ui->listWidgetLibs->sortItems();
  if (!files.empty())
  {
    settings.setValue(_prefix + "::load_dir", QFileInfo(files.front()).dir().absolutePath());
  }
}

void ConfigZCM::on_pushButtonRemove_clicked()
{
  auto items = ui->listWidgetLibs->selectedItems();
  for (auto item : items)
  {
    ui->listWidgetLibs->removeItemWidget(item);
    delete item;
  }
}
