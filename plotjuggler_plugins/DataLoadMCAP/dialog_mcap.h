#ifndef DIALOG_MCAP_H
#define DIALOG_MCAP_H

#include <memory>
#include <QDialog>
#include <optional>
#include "dataload_params.h"

namespace Ui
{
class dialog_mcap;
}

namespace mcap
{
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
class Schema;
using SchemaPtr = std::shared_ptr<Schema>;
class LoadParams;
}  // namespace mcap

class DialogMCAP : public QDialog
{
  Q_OBJECT

public:
  explicit DialogMCAP(const std::unordered_map<int, mcap::ChannelPtr>& channels,
                      const std::unordered_map<int, mcap::SchemaPtr>& schemas,
                      std::optional<mcap::LoadParams> default_parameters,
                      QWidget* parent = nullptr);
  ~DialogMCAP();

  mcap::LoadParams getParams() const;

private slots:
  void on_tableWidget_itemSelectionChanged();
  void accept() override;

private:
  Ui::dialog_mcap* ui;

  static const QString prefix;
};

#endif  // DIALOG_MCAP_H
