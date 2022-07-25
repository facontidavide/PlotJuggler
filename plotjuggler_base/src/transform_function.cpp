/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "PlotJuggler/transform_function.h"

namespace PJ
{
TransformFunction::TransformFunction() : _data(nullptr)
{
  static unsigned order = 0;
  _order = order++;
}

std::vector<const PlotData*>& TransformFunction::dataSources()
{
  return _src_vector;
}

void TransformFunction::setData(PlotDataMapRef* data,
                                const std::vector<const PlotData*>& src_vect,
                                std::vector<PlotData*>& dst_vect)
{
  if (numInputs() >= 0 && src_vect.size() != numInputs())
  {
    throw std::runtime_error("Wrong number of input data sources "
                             "in setDataSource");
  }
  if (numOutputs() >= 0 && dst_vect.size() != numOutputs())
  {
    throw std::runtime_error("Wrong number of output data destinations");
  }
  _data = data;
  _src_vector = src_vect;
  _dst_vector = dst_vect;
}

void TransformFunction_SISO::reset()
{
  _last_timestamp = std::numeric_limits<double>::lowest();
}

void TransformFunction_SISO::calculate()
{
  const PlotData* src_data = _src_vector.front();
  PlotData* dst_data = _dst_vector.front();
  if (src_data->size() == 0)
  {
    return;
  }
  dst_data->setMaximumRangeX(src_data->maximumRangeX());
  if (dst_data->size() != 0)
  {
    _last_timestamp = dst_data->back().x;
  }

  int pos = src_data->getIndexFromX(_last_timestamp);
  size_t index = pos < 0 ? 0 : static_cast<size_t>(pos);

  while (index < src_data->size())
  {
    const auto& in_point = src_data->at(index);

    if (in_point.x >= _last_timestamp)
    {
      auto out_point = calculateNextPoint(index);
      if (out_point)
      {
        dst_data->pushBack(std::move(out_point.value()));
      }
      _last_timestamp = in_point.x;
    }
    index++;
  }
}

TransformFunction::Ptr TransformFactory::create(const std::string& name)
{
  auto it = instance()->creators_.find(name);
  if (it == instance()->creators_.end())
  {
    return {};
  }
  return it->second();
}

TransformFactory* PJ::TransformFactory::instance()
{
  static TransformFactory* _ptr(nullptr);
  if (!qApp->property("TransformFactory").isValid() && !_ptr)
  {
    _ptr = _transform_factory_ptr_from_macro;
    qApp->setProperty("TransformFactory", QVariant::fromValue(_ptr));
  }
  else if (!_ptr)
  {
    _ptr = qvariant_cast<TransformFactory*>(qApp->property("TransformFactory"));
  }
  else if (!qApp->property("TransformFactory").isValid())
  {
    qApp->setProperty("TransformFactory", QVariant::fromValue(_ptr));
  }
  return _ptr;
}

const std::set<std::string>& TransformFactory::registeredTransforms()
{
  return instance()->names_;
}

const PlotData* TransformFunction_SISO::dataSource() const
{
  if (_src_vector.empty())
  {
    return nullptr;
  }
  return _src_vector.front();
}

}  // namespace PJ
