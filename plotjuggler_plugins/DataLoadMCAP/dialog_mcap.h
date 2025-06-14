#ifndef DIALOG_MCAP_H
#define DIALOG_MCAP_H

#include <memory>
#include <QDialog>
#include <optional>
#include "dataload_params.h"
#include <QShortcut>

namespace Ui
{
class dialog_mcap;
}

namespace mcap
{
struct Channel;
using ChannelPtr = std::shared_ptr<Channel>;
struct Schema;
using SchemaPtr = std::shared_ptr<Schema>;
struct LoadParams;

}  // namespace mcap

class DialogMCAP : public QDialog
{
  Q_OBJECT

public:
  explicit DialogMCAP(
      const std::unordered_map<int, mcap::ChannelPtr>& channels,
      const std::unordered_map<int, mcap::SchemaPtr>& schemas,
      const std::unordered_map<uint16_t, uint64_t>& messages_count_by_channelID,
      std::optional<mcap::LoadParams> default_parameters, QWidget* parent = nullptr);
  ~DialogMCAP();

  mcap::LoadParams getParams() const;

private slots:
  void on_tableWidget_itemSelectionChanged();
  void accept() override;

private:
  Ui::dialog_mcap* ui;

  static const QString prefix;

  QShortcut _select_all;
  QShortcut _deselect_all;
};

#endif  // DIALOG_MCAP_H
