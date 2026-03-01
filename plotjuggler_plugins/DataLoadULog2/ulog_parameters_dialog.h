#pragma once

#include <QDialog>
#include <QString>

namespace ulog_cpp
{
class DataContainer;
}

namespace Ui
{
class ULogParametersDialog;
}

class ULogParametersDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ULogParametersDialog(const ulog_cpp::DataContainer& container,
                                QWidget* parent = nullptr);

  void restoreSettings();

  ~ULogParametersDialog();

private:
  Ui::ULogParametersDialog* ui;
};
