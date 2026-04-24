#pragma once

#include <QWidget>

class DropOverlay : public QWidget
{
  Q_OBJECT

public:
  enum class Mode { Hidden, Ready, NoWritePermission, NotConnected };

  explicit DropOverlay(QWidget* parent);

  void setMode(Mode mode);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  Mode mode_ = Mode::Hidden;
};
