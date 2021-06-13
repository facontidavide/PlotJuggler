#ifndef CHEATSHEET_DIALOG_H
#define CHEATSHEET_DIALOG_H

#include <QDialog>

namespace Ui {
class CheatsheetDialog;
}

class CheatsheetDialog : public QDialog
{
  Q_OBJECT

public:
  explicit CheatsheetDialog(QWidget *parent = nullptr);
  ~CheatsheetDialog();

private slots:
  void on_listWidget_currentRowChanged(int currentRow);

private:
  Ui::CheatsheetDialog *ui;
};

#endif // CHEATSHEET_DIALOG_H
