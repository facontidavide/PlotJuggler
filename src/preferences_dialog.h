#ifndef PREFERENCES_DIALOG_H
#define PREFERENCES_DIALOG_H

#include <QDialog>

namespace Ui
{
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
  Q_OBJECT

public:
  explicit PreferencesDialog(QWidget* parent = nullptr);
  ~PreferencesDialog();

private slots:
  void on_buttonBox_accepted();

  void on_pushButtonAdd_clicked();

  void on_pushButtonRemove_clicked();

  void on_listWidget_itemSelectionChanged();

private:
  Ui::PreferencesDialog* ui;
};

#endif  // PREFERENCES_DIALOG_H
