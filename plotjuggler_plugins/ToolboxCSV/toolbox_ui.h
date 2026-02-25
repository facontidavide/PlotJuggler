#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>
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
  std::vector<std::string> getSelectedTopics() const;
  QString getPath() const;

  void setPath(const QString filePath);

  bool isRelativeBox() const;
  bool isCheckBoxTime() const;
  bool isCsvButton() const;

  void clearTable(bool clearAll);

  void updateTimeControlsEnabled();

  // Apply time range to slider and spinboxes
  void setTimeRange(double tmin, double tmax);

  QWidget* widget() const;

signals:
  void removeRequested();
  void clearRequested();
  void closed();
  void saveRequested();
  void pickFileRequested();
  void recomputeTime();

private:
  Ui::toolBoxUI* ui = nullptr;
  QWidget* _widget;

  // Temporary storage for drag & drop topics
  QStringList _dragging_curves;

  // Reference time offset for relative mode
  double _t0 = 0.0;

  bool eventFilter(QObject* obj, QEvent* ev) override;
};
