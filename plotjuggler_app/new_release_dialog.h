#ifndef NEW_RELEASE_DIALOG_H
#define NEW_RELEASE_DIALOG_H

#include <QDialog>

namespace Ui
{
class NewReleaseDialog;
}

class NewReleaseDialog : public QDialog
{
  Q_OBJECT

public:
  NewReleaseDialog(QWidget* parent, QString release, QString title, QString url);
  ~NewReleaseDialog();

private:
  Ui::NewReleaseDialog* ui;
};

#endif  // NEW_RELEASE_DIALOG_H
