#include "menubar.h"
#include <QDebug>
#include <QFontDatabase>
#include <QFont>
#include <QColor>
#include <QFontMetrics>

MenuBar::MenuBar(QWidget* parent) : QMenuBar(parent)
{
  int font_id = QFontDatabase::addApplicationFont("://resources/DejaVuSans-ExtraLight.ttf");
  QString family = QFontDatabase::applicationFontFamilies(font_id).at(0);

  _font.setFamily(family);
  _font.setStyleStrategy(QFont::PreferAntialias);
  _font.setPixelSize(18);

  QFontMetrics fm(_font);
  _width_plot = fm.width("Plot");
  _width_juggler = fm.width("Juggler");
}

//void MenuBar::paintEvent(QPaintEvent* event)
//{
//  QMenuBar::paintEvent(event);
//  QPainter painter(this);
//  painter.setFont(_font);

//  int text_width = _width_plot + _width_juggler;
//  {
//    QPoint topleft(this->rect().width() - text_width - 12, 0);
//    QSize rect_size(_width_plot, this->rect().height());
//    painter.setPen(QColor("#dd22dd"));
//    painter.drawText(QRect(topleft, rect_size), Qt::AlignHCenter | Qt::AlignVCenter, "Plot");
//  }
//  {
//    QPoint topleft(this->rect().width() - _width_juggler - 10, 0);
//    QSize rect_size(_width_juggler, this->rect().height());
//    painter.setPen(QColor("#2288ff"));
//    painter.drawText(QRect(topleft, rect_size), Qt::AlignHCenter | Qt::AlignVCenter, "Juggler");
//  }
//}
