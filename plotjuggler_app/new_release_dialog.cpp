/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

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

  connect(ui->pushButtonWeb, &QPushButton::clicked, this, [=] {
    QDesktopServices::openUrl(QUrl(url));
    link_opened = true;
  });

  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, [=] {
    if (ui->dontShowAgain->isChecked())
    {
      QSettings settings;
      settings.setValue("NewRelease/dontShowThisVersion", release);
    }
  });

  ui->labelTitle->setText(title);
}

NewReleaseDialog::~NewReleaseDialog()
{
  delete ui;
}
