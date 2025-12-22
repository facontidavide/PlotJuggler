/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "utils.h"
#include <QDebug>

namespace PJ
{

template <typename Value>
void MergeData(TimeseriesBase<Value>& src_plot, TimeseriesBase<Value>& dst_plot)
{
  if (src_plot.size() == 0)
  {
    return;
  }
  if (dst_plot.size() == 0)
  {
    std::swap(dst_plot, src_plot);
    return;
  }

  // special case: is the same data being overwritten?
  if (dst_plot.size() == src_plot.size() && isEqual(dst_plot.back().x, src_plot.back().x) &&
      isEqual(dst_plot.front().x, src_plot.front().x))
  {
    bool need_sorting = false;
    for (size_t i = 0; i < src_plot.size(); i++)
    {
      auto& src_point = src_plot[i];
      auto& dst_point = dst_plot[i];
      if (isEqual(src_point.x, dst_point.x))
      {
        // update only
        dst_point.y = src_point.y;
      }
      else
      {
        dst_plot.pushUnsorted(std::move(src_point));
        need_sorting = true;
      }
    }
    src_plot.clear();
    if (need_sorting)
    {
      dst_plot.sort();
    }
    return;
  }

  // append
  if (dst_plot.back().x < src_plot.front().x)
  {
    for (size_t i = 0; i < src_plot.size(); i++)
    {
      dst_plot.pushBack(std::move(src_plot.at(i)));
    }
    src_plot.clear();
    return;
  }
  // prepend
  if (src_plot.back().x < dst_plot.front().x)
  {
    // swap and append
    std::swap(dst_plot, src_plot);
    for (size_t i = 0; i < src_plot.size(); i++)
    {
      dst_plot.pushBack(std::move(src_plot.at(i)));
    }
    src_plot.clear();
    return;
  }
  // LAST CASE: merging
  for (size_t i = 0; i < src_plot.size(); i++)
  {
    dst_plot.pushUnsorted(std::move(src_plot.at(i)));
  }
  dst_plot.sort();
  src_plot.clear();
}

void MergeData(PlotDataXY& src_plot, PlotDataXY& dst_plot)
{
  if (dst_plot.size() == 0)
  {
    std::swap(dst_plot, src_plot);
    return;
  }
  for (const auto& p : src_plot)
  {
    dst_plot.pushBack(p);
  }
  src_plot.clear();
}

MoveDataRet MoveData(PlotDataMapRef& source, PlotDataMapRef& destination, bool remove_older)
{
  MoveDataRet ret;

  auto moveDataImpl = [&](auto& source_series, auto& destination_series) {
    for (auto& [source_ID, source_plot] : source_series)
    {
      const std::string& plot_name = source_plot.plotName();

      auto dest_plot_it = destination_series.find(source_ID);
      if (dest_plot_it == destination_series.end())
      {
        ret.added_curves.push_back(source_ID);

        PlotGroup::Ptr group;
        if (source_plot.group())
        {
          destination.getOrCreateGroup(source_plot.group()->name());
        }
        dest_plot_it = destination_series
                           .emplace(std::piecewise_construct, std::forward_as_tuple(source_ID),
                                    std::forward_as_tuple(plot_name, group))
                           .first;
        ret.curves_updated = true;
      }

      auto& destination_plot = dest_plot_it->second;
      PlotGroup::Ptr destination_group = destination_plot.group();

      // copy plot attributes
      for (const auto& [name, attr] : source_plot.attributes())
      {
        if (destination_plot.attribute(name) != attr)
        {
          destination_plot.setAttribute(name, attr);
          ret.curves_updated = true;
        }
      }
      // Copy the group name and attributes
      if (source_plot.group())
      {
        if (!destination_group || destination_group->name() != source_plot.group()->name())
        {
          destination_group = destination.getOrCreateGroup(source_plot.group()->name());
          destination_plot.changeGroup(destination_group);
        }

        for (const auto& [name, attr] : source_plot.group()->attributes())
        {
          if (destination_group->attribute(name) != attr)
          {
            destination_group->setAttribute(name, attr);
            ret.curves_updated = true;
          }
        }
      }

      if (remove_older)
      {
        destination_plot.clear();
      }

      if constexpr (std::is_same_v<PlotData, decltype(source_plot)> ||
                    std::is_same_v<StringSeries, decltype(source_plot)> ||
                    std::is_same_v<PlotDataAny, decltype(source_plot)>)
      {
        double max_range_x = source_plot.maximumRangeX();
        destination_plot.setMaximumRangeX(max_range_x);
      }
      MergeData(source_plot, destination_plot);
    }
  };

  //--------------------------------------------
  moveDataImpl(source.numeric, destination.numeric);
  moveDataImpl(source.strings, destination.strings);
  moveDataImpl(source.scatter_xy, destination.scatter_xy);
  moveDataImpl(source.user_defined, destination.user_defined);

  return ret;
}

}  // namespace PJ
