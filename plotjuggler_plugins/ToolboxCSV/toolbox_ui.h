#pragma once

#include <QObject>
#include <QWidget>
#include <QDir>
#include <QString>
#include "PlotJuggler/plotdatabase.h"
#include <QStringList>
#include <QEvent>
#include <vector>
#include <string>

namespace Ui
{
class toolBoxUI;
}

class ToolBoxUI : public QObject
{
  Q_OBJECT
public:
  ToolBoxUI();

  double getStartTime() const;
  double getEndTime() const;
  double getRelativeTime() const;

  // Get absolute time range (adjusting for relative mode offset)
  PJ::Range getAbsoluteTimeRange() const;

  std::vector<std::string> getSelectedTopics() const;

  // Return the prefix used to create multiple files
  QString getPathPrefix() const;

  bool isRelativeTime() const;

  void clearTable(bool clearAll);

  void updateTimeControlsEnabled();

  // Apply time range to slider and spinboxes
  void setTimeRange(double tmin, double tmax);

  QWidget* widget() const;

signals:
  void removeRequested();
  void clearRequested();
  void closed();
  void recomputeTime();

  /**
   * @brief exportSingleFile will export all data in a single file.
   *
   * @param has_csv   if false, export as parquet
   * @param filename  file name of the exported file
   */
  void exportSingleFile(bool has_csv, QString filename);

  /**
   * @brief exportMultipleFiles
   *
   * @param has_csv          if false, export as parquet
   * @param destination_dir  directory where the files should be saved
   * @param filename_prefix  all files should have this prefix. the suffic is the group name
   */
  void exportMultipleFiles(bool has_csv, QDir destination_dir, QString filename_prefix);

private:
  Ui::toolBoxUI* ui = nullptr;
  QWidget* _widget;

  // Temporary storage for drag & drop topics
  QStringList _dragging_curves;

  // Reference time offset for relative mode
  double _time_offset = 0.0;

  bool eventFilter(QObject* obj, QEvent* ev) override;
};
