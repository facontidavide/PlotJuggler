#pragma once

#include <QDialog>
#include <QString>
#include "ulog_parser.h"

namespace Ui
{
class ULogParametersDialog;
}

class ULogParametersDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ULogParametersDialog(const ULogParser& parser, QWidget* parent = nullptr);

  void restoreSettings();

  ~ULogParametersDialog();

private:
  Ui::ULogParametersDialog* ui;
};

