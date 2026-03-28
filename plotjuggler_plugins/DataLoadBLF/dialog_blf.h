#pragma once

#include <QDialog>

#include "blf_config.h"

namespace Ui
{
class DialogBLF;
}

namespace PJ::BLF
{

class DialogBLF : public QDialog
{
  Q_OBJECT

public:
  explicit DialogBLF(QWidget* parent = nullptr);
  ~DialogBLF() override;

  void SetConfig(const BlfPluginConfig& config);
  BlfPluginConfig GetConfig() const;

private slots:
  void OnAddDbc();
  void OnRemoveDbc();
  void OnAddMapRow();
  void OnRemoveMapRow();
  void accept() override;

private:
  void AppendMapRow(uint32_t channel, const QString& dbc_path);

  Ui::DialogBLF* ui_;
};

}  // namespace PJ::BLF
