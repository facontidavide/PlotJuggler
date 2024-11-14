#pragma once

#include <QWidget>

namespace Ui
{
class ConfigZCM;
}

class ConfigZCM : public QWidget
{
  Q_OBJECT

public:
  explicit ConfigZCM(QString prefix, QWidget* parent = nullptr);
  ~ConfigZCM();

  QString getLibraries() const;
  QString getDataPrefix() const;

private slots:
  void on_radioEnvironmentLibrary_toggled(bool checked);

  void on_radioManualLibrary_toggled(bool checked);

  void on_pushButtonAdd_clicked();

  void on_pushButtonRemove_clicked();

private:
  Ui::ConfigZCM* ui;
  QString _prefix;
};
