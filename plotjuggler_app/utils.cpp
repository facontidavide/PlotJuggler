#include "utils.h"
#include <QDebug>

MoveDataRet MoveData(PlotDataMapRef& source, PlotDataMapRef& destination,
                     bool remove_older)
{
  MoveDataRet ret;

  auto moveDataImpl = [&](auto& source_series, auto& destination_series) {
    for (auto& it : source_series)
    {
      const std::string& ID = it.first;
      auto& source_plot = it.second;
      const std::string& plot_name = source_plot.plotName();

      auto dest_plot_it = destination_series.find(ID);
      if (dest_plot_it == destination_series.end())
      {
        ret.added_curves.push_back(ID);

        PlotGroup::Ptr group;
        if (source_plot.group())
        {
          destination.getOrCreateGroup(source_plot.group()->name());
        }
        dest_plot_it = destination_series
                           .emplace(std::piecewise_construct, std::forward_as_tuple(ID),
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
        if (!destination_group ||
            destination_group->name() != source_plot.group()->name())
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

      if (source_plot.size() > 0)
      {
        ret.data_pushed = true;
      }

      for (size_t i = 0; i < source_plot.size(); i++)
      {
        destination_plot.pushBack(source_plot.at(i));
      }

      if constexpr( std::is_same_v<PlotData, decltype (source_plot)> ||
                    std::is_same_v<StringSeries, decltype (source_plot)> ||
                    std::is_same_v<PlotDataAny, decltype (source_plot)> )
      {
        double max_range_x = source_plot.maximumRangeX();
        destination_plot.setMaximumRangeX(max_range_x);
      }

      source_plot.clear();
    }
  };

  //--------------------------------------------
  moveDataImpl(source.numeric, destination.numeric);
  moveDataImpl(source.strings, destination.strings);
  moveDataImpl(source.scatter_xy, destination.scatter_xy);
  moveDataImpl(source.user_defined, destination.user_defined);

  return ret;
}
