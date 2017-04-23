#ifndef RANDOM_COLOR_H
#define RANDOM_COLOR_H

#include <QColor>

inline QColor randomColorHint()
{
    static int index = 0;
    QColor color;
    switch( index%9 )
    {
    case 0:  color =  QColor(Qt::blue);       break;
    case 1:  color =  QColor(Qt::darkGreen);  break;
    case 2:  color =  QColor(Qt::red);        break;
    case 3:  color =  QColor(Qt::magenta);    break;
    case 4:  color =  QColor(Qt::darkBlue);   break;
    case 5:  color =  QColor(Qt::darkCyan);   break;
    case 6:  color =  QColor(Qt::gray);       break;
    case 7:  color =  QColor(Qt::darkYellow); break;
    case 8:  color =  QColor(Qt::darkRed);    break;
    }
    index++;
    return color;
}

#endif // RANDOM_COLOR_H
