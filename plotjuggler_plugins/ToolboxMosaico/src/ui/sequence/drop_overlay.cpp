#include "drop_overlay.h"

#include "../colors.h"

#include <QPainter>
#include <QPaintEvent>

DropOverlay::DropOverlay(QWidget* parent) : QWidget(parent)
{
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setAttribute(Qt::WA_NoSystemBackground);
  hide();
}

void DropOverlay::setMode(Mode mode)
{
  if (mode_ == mode) return;
  mode_ = mode;
  setVisible(mode != Mode::Hidden);
  update();
}

void DropOverlay::paintEvent(QPaintEvent* /*event*/)
{
  if (mode_ == Mode::Hidden) return;

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  bool is_error = (mode_ == Mode::NoWritePermission ||
                   mode_ == Mode::NotConnected);
  QColor border = is_error ? kErrorRed
                           : palette().color(QPalette::Highlight);
  QColor fill = border;
  fill.setAlpha(20);

  p.fillRect(rect(), fill);

  QPen pen(border, 2, Qt::DashLine);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(rect().adjusted(4, 4, -4, -4), 6, 6);

  QString label;
  switch (mode_)
  {
    case Mode::Ready:
      label = "Drop MCAP/DB3/BAG files here to upload";
      break;
    case Mode::NoWritePermission:
      label = "No Write Permissions";
      break;
    case Mode::NotConnected:
      label = "Not connected";
      break;
    case Mode::Hidden:
      return;
  }

  QFont f = font();
  f.setBold(true);
  f.setPointSize(f.pointSize() + 2);
  p.setFont(f);
  p.setPen(border);
  p.drawText(rect(), Qt::AlignCenter, label);
}
