#pragma once

#include <QApplication>
#include <set>
#include <functional>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/pj_plugin.h"

namespace PJ {

class TimeSeriesTransform : public PlotJugglerPlugin
{
  Q_OBJECT
public:

  TimeSeriesTransform(): _src_data(nullptr)
  {
    init();
  }

  virtual ~TimeSeriesTransform() {}

  void setDataSource(const PlotData *src_data){
    _src_data = src_data;
  }

  virtual void init()
  {
    _last_timestamp = - std::numeric_limits<double>::max();
  }

  virtual const char* name() const = 0;

  void calculate(PlotData* dst_data)
  {
    if (_src_data->size() == 0)
    {
      return;
    }
    dst_data->setMaximumRangeX( _src_data->maximumRangeX() );
    if (dst_data->size() != 0)
    {
      _last_timestamp = dst_data->back().x;
    }

    int pos = _src_data->getIndexFromX( _last_timestamp );
    size_t index = pos < 0 ? 0 : static_cast<size_t>(pos);

    while(index < _src_data->size())
    {
      const auto& in_point = _src_data->at(index);

      if (in_point.x >= _last_timestamp)
      {
        auto out_point = calculateNextPoint(index);
        if (out_point){
          dst_data->pushBack( out_point.value() );
        }
        _last_timestamp = in_point.x;
      }
      index++;
    }
  }

  const PlotData* dataSource() const{
    return _src_data;
  }

  QString alias() const {
    return _alias;
  }

  void setAlias(QString alias) {
    _alias = alias;
  }

signals:
  void parametersChanged();

protected:

  const PlotData *_src_data;
  QString _alias;
  double _last_timestamp;

  virtual std::optional<PlotData::Point>
  calculateNextPoint(size_t index) = 0;
};

using TimeSeriesTransformPtr = std::shared_ptr<TimeSeriesTransform>;

///------ The factory to create instances of a SeriesTransform -------------

class TransformFactory: public QObject
{
public:
  TransformFactory() {}

private:
  TransformFactory(const TransformFactory&) = delete;
  TransformFactory& operator=(const TransformFactory&) = delete;

  std::map<std::string, std::function<TimeSeriesTransformPtr()>> creators_;
  std::set<std::string> names_;

  static TransformFactory* instance();

public:

  static const std::set<std::string>& registeredTransforms() {
    return instance()->names_;
  }

  template <typename T> static void registerTransform()
  {
    T temp;
    std::string name = temp.name();
    instance()->names_.insert(name);
    instance()->creators_[name] = [](){ return std::make_shared<T>(); };
  }

  static TimeSeriesTransformPtr create(const std::string& name)
  {
    auto it = instance()->creators_.find(name);
    if( it == instance()->creators_.end())
    {
      return {};
    }
    return it->second();
  }
};

} // end namespace

Q_DECLARE_OPAQUE_POINTER(PJ::TransformFactory *)
Q_DECLARE_METATYPE(PJ::TransformFactory *)
Q_GLOBAL_STATIC(PJ::TransformFactory, _transform_factory_ptr_from_macro)

inline PJ::TransformFactory* PJ::TransformFactory::instance()
{
  static TransformFactory * _ptr(nullptr);
  if (!qApp->property("TransformFactory").isValid() && !_ptr) {
    _ptr = _transform_factory_ptr_from_macro;
    qApp->setProperty("TransformFactory", QVariant::fromValue(_ptr));
  }
  else if (!_ptr) {
    _ptr = qvariant_cast<TransformFactory *>(qApp->property("TransformFactory"));
  }
  else if (!qApp->property("TransformFactory").isValid()) {
    qApp->setProperty("TransformFactory", QVariant::fromValue(_ptr));
  }
  return _ptr;
}


QT_BEGIN_NAMESPACE
#define TimeSeriesTransform_iid "facontidavide.PlotJuggler3.TimeSeriesTransform"
Q_DECLARE_INTERFACE(PJ::TimeSeriesTransform, TimeSeriesTransform_iid)
QT_END_NAMESPACE

