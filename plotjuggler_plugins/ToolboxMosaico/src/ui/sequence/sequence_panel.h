#pragma once

#include <flight/types.hpp>
using mosaico::SequenceInfo;

#include "../picker/sequence_picker_widget.h"

#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSet>
#include <QString>
#include <QWidget>

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

class QTableWidget;
class QTableWidgetItem;

class DropOverlay;

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
  void setPermissions(bool can_write, bool can_delete);
  void setConnected(bool connected);

  void onUploadStarted(const QString& sequence_name);
  void onUploadProgress(const QString& sequence_name, int percent);
  void onUploadSucceeded(const QString& sequence_name);
  void onUploadFailed(const QString& sequence_name, const QString& message);

  void onDismiss(const QString& sequence_name);

  // Returns the set of sequence names currently in the table
  // (for collision detection by callers).
  QStringList existingSequenceNames() const;

signals:
  void sequenceSelected(const QString& sequence_name);
  void filesDropped(const QStringList& paths);
  void deleteRequested(const QStringList& names);
  void dismissRequested(const QString& name);

private slots:
  void onCellClicked(int row, int column);
  void applyFilter();
  void onFilterChanged(const RangeFilter& filter);

protected:
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragLeaveEvent(QDragLeaveEvent* event) override;
  void dropEvent(QDropEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
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
  DropOverlay* overlay_ = nullptr;
  bool can_write_ = false;
  bool can_delete_ = false;
  bool is_connected_ = false;

  // Map sequence_name -> name-column item pointer for pending upload rows.
  // Using item pointers instead of row indices so that table sorting
  // does not invalidate the mapping.
  // Failed rows stay in the map until dismissed.
  QHash<QString, QTableWidgetItem*> pending_rows_;
  QSet<QString> failed_rows_;  // subset of pending_rows_ that failed
};
