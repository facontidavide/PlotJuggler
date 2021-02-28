#include "utils.h"

/*
std::vector<double> BuiltTimepointsList(PlotDataMapRef &data)
{
    {
        std::vector<size_t> index_num( data.numeric.size(), 0);

        std::vector<double> out;

        bool loop = true;

        const double MAX = std::numeric_limits<double>::max();

        while(loop)
        {
            double min_time = MAX;
            double prev_time = out.empty() ? -MAX : out.back();
            loop = false;
            size_t count = 0;
            for(const auto& it: data.numeric)
            {
                const auto& plot = it.second;
                out.reserve(plot.size());

                size_t index = index_num[count];

                while ( index < plot.size() && plot.at(index).x <= prev_time   )
                {
                    index++;
                }
                if( index >= plot.size() )
                {
                    count++;
                    continue;
                }
                else{
                    loop = true;
                }
                index_num[count] = index;
                double time_val = plot.at(index).x;
                min_time = std::min( min_time, time_val);
                count++;
            }
            if( min_time < MAX)
            {
                out.push_back( min_time );
            }
        }
        return out;
    }
}
*/

std::pair<std::vector<QString>, bool> MoveData(PlotDataMapRef &source,
                                               PlotDataMapRef &destination)
{
  bool destination_updated = false;
  std::vector<QString> added_curves;

  for (auto& it : source.numeric)
  {
    const std::string& name = it.first;
    auto& source_plot = it.second;
    if(source_plot.size() == 0)
    {
      continue;
    }

    auto plot_with_same_name = destination.numeric.find(name);

    // this is a new plot
    if (plot_with_same_name == destination.numeric.end())
    {
      added_curves.push_back(QString::fromStdString(name));

      plot_with_same_name =
          destination.numeric
          .emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name))
          .first;
    }
    auto& destination_plot = plot_with_same_name->second;
    for (size_t i = 0; i < source_plot.size(); i++)
    {
      destination_plot.pushBack(source_plot.at(i));
      destination_updated = true;
    }
    source_plot.clear();
  }

  for (auto& it : source.user_defined)
  {
    const std::string& name = it.first;
    auto& source_plot = it.second;
    auto plot_with_same_name = destination.user_defined.find(name);

    // this is a new plot
    if (plot_with_same_name == destination.user_defined.end())
    {
      plot_with_same_name =
          destination.user_defined
          .emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(name))
          .first;
    }
    auto& destination_plot = plot_with_same_name->second;
    for (size_t i = 0; i < source_plot.size(); i++)
    {
      destination_plot.pushBack( std::move(source_plot.at(i)) );
      destination_updated = true;
    }
    source_plot.clear();
  }
  return { added_curves, destination_updated };
}
