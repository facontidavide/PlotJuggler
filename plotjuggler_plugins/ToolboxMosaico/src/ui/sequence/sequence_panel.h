/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <flight/types.hpp>
using mosaico::SequenceInfo;

#include "../picker/sequence_picker_widget.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QWidget>

#include <optional>
#include <set>
#include <string>
#include <vector>

class QTableWidget;
class QTableWidgetItem;
class QTimer;

class SequencePanel : public QWidget
{
  Q_OBJECT

public:
  explicit SequencePanel(QWidget* parent = nullptr);

public slots:
  void populateSequences(const std::vector<SequenceInfo>& sequences);
  void updateSequence(const SequenceInfo& sequence);
  void setMetadataLoadingProgress(qint64 loaded, qint64 total);
  void setLoading(bool loading);
  void setVisibleSequences(const std::set<std::string>& visible_names);
  void clearVisibleSequences();

signals:
  void sequenceSelected(const QString& sequence_name);

private slots:
  void onCellClicked(int row, int column);
  void applyFilter();
  void onFilterChanged(const RangeFilter& filter);

protected:
  void contextMenuEvent(QContextMenuEvent* event) override;

private:
  static QString formatDate(int64_t ts_ns);
  void setSequenceRow(int row, const SequenceInfo& seq);
  int findRowByName(const QString& name) const;
  void refreshEarliestDate();
  void updateHeader();

  QLabel* header_ = nullptr;
  QLineEdit* filter_ = nullptr;
  QPushButton* regex_btn_ = nullptr;
  SequencePickerWidget* picker_ = nullptr;
  QTableWidget* table_ = nullptr;
  QTimer* loading_timer_ = nullptr;
  std::vector<SequenceInfo> all_sequences_;
  RangeFilter range_filter_;
  bool list_loading_ = false;
  bool metadata_loading_ = false;
  bool sequence_list_populated_ = false;
  int loading_frame_ = 0;
  int visible_count_ = 0;
  int total_count_ = 0;
  qint64 metadata_loaded_ = 0;
  qint64 metadata_total_ = 0;
  // nullopt = no metadata filter active (show all).
  // empty set = filter active, nothing matches (show none).
  std::optional<std::set<std::string>> visible_sequences_;
};
