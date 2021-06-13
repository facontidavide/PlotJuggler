#include "new_release_dialog.h"
#include "ui_new_release_dialog.h"
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QDialogButtonBox>

NewReleaseDialog::NewReleaseDialog(QWidget* parent, QString release, QString title, QString url)
  : QDialog(parent), ui(new Ui::NewReleaseDialog)
{
  ui->setupUi(this);
  setWindowFlags(Qt::WindowStaysOnTopHint);

  connect(ui->pushButtonWeb, &QPushButton::clicked, this, [=] { QDesktopServices::openUrl(QUrl(url)); });

  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, [=] {
    if (ui->dontShowAgain->isChecked())
    {
      QSettings settings;
      settings.setValue("NewRelease/dontShowThisVersion", release);
    }
  });

  ui->labelRelease->setText(release);
  ui->labelTitle->setText(title);
}

NewReleaseDialog::~NewReleaseDialog()
{
  delete ui;
}
