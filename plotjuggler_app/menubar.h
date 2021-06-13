#ifndef MENUBAR_H
#define MENUBAR_H

#include <QMenuBar>
#include <QPaintEvent>
#include <QPainter>

class MenuBar : public QMenuBar
{
public:
  MenuBar(QWidget* parent);
 // void paintEvent(QPaintEvent* event);

private:
  QFont _font;
  int _width_plot;
  int _width_juggler;
};

#endif  // MENUBAR_H
