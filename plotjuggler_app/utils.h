#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include "PlotJuggler/plotdata.h"

using namespace PJ;

class MonitoredValue : public QObject
{
  Q_OBJECT
public:
  MonitoredValue(QObject* parent = nullptr) : QObject(parent), _value(0)
  {
  }

  void set(double newValue)
  {
    double prev = _value;
    _value = newValue;
    if (fabs(newValue - prev) > std::numeric_limits<double>::epsilon())
    {
      emit valueChanged(_value);
    }
  }

  double get() const
  {
    return _value;
  }
signals:
  void valueChanged(double);

private:
  double _value;
};


struct MoveDataRet
{
  std::vector<std::string> added_curves;
  bool curves_updated = false;
  bool data_pushed = false;
};


MoveDataRet MoveData(PlotDataMapRef& source,
            PlotDataMapRef& destination,
            bool remove_older);


#endif  // UTILS_H
