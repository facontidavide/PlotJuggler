#ifndef DIALOG_MCAP_H
#define DIALOG_MCAP_H

#include <QDialog>

#include "mcap/reader.hpp"

namespace Ui {
class dialog_mcap;
}

class DialogMCAP : public QDialog
{
  Q_OBJECT

public:

  struct Params
  {
    QStringList selected_topics;
    unsigned max_array_size;
    bool clamp_large_arrays;
  };

  explicit DialogMCAP(const  std::unordered_map<int, mcap::ChannelPtr>& channels,
                      const std::unordered_map<int, mcap::SchemaPtr>& schemas,
                      QWidget *parent = nullptr);
  ~DialogMCAP();

  Params getParams() const;

private slots:
  void on_tableWidget_itemSelectionChanged();
  void accept() override;

private:
  Ui::dialog_mcap *ui;

  static const QString prefix;
};

#endif // DIALOG_MCAP_H
