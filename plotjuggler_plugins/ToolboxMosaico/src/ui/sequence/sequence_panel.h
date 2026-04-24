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

class SequencePanel : public QWidget
{
  Q_OBJECT

public:
  explicit SequencePanel(QWidget* parent = nullptr);

public slots:
  void populateSequences(const std::vector<SequenceInfo>& sequences);
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

  QLabel* header_ = nullptr;
  QLineEdit* filter_ = nullptr;
  QPushButton* regex_btn_ = nullptr;
  SequencePickerWidget* picker_ = nullptr;
  QTableWidget* table_ = nullptr;
  std::vector<SequenceInfo> all_sequences_;
  RangeFilter range_filter_;
  // nullopt = no metadata filter active (show all).
  // empty set = filter active, nothing matches (show none).
  std::optional<std::set<std::string>> visible_sequences_;
};
