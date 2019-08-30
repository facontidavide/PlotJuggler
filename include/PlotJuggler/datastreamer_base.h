#ifndef DATA_STREAMER_TEMPLATE_H
#define DATA_STREAMER_TEMPLATE_H

#include <mutex>
#include <unordered_set>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/pj_plugin.h"

/**
 * @brief The DataStreamer base class to create your own plugin.
 *
 * Important. To avoid problems with thread safety, it is important that ANY update to
 * dataMap(), which share its elements with the main application, is protected by the mutex()
 *
 * This includes in particular the periodic updates.
 */
class DataStreamer: public PlotJugglerPlugin
{
    Q_OBJECT
public:

    DataStreamer():
        PlotJugglerPlugin()
    {

    }

    virtual bool start(QStringList*) = 0;

    virtual bool start(QStringList*, const bool&) = 0;

    virtual void shutdown() = 0;

    virtual bool isRunning() const = 0;

    virtual ~DataStreamer() {}

    std::mutex& mutex()
    {
        return _mutex;
    }

    void setMaximumRange(double range);

    virtual std::vector<QString> appendData(PlotDataMapRef& destination);

    PlotDataMapRef& dataMap()
    {
        return _data_map;
    }

    const PlotDataMapRef& dataMap() const
    {
        return _data_map;
    }

signals:

    void clearBuffers();

    void dataUpdated();

    void connectionClosed();

private:
    std::mutex _mutex;
    PlotDataMapRef _data_map;
    QAction* _start_streamer;
};

QT_BEGIN_NAMESPACE

#define DataStream_iid "com.icarustechnology.PlotJuggler.DataStreamer"



Q_DECLARE_INTERFACE(DataStreamer, DataStream_iid)

QT_END_NAMESPACE


inline
void DataStreamer::setMaximumRange(double range)
{
    std::lock_guard<std::mutex> lock( mutex() );
    for (auto& it : dataMap().numeric ) {
        it.second.setMaximumRangeX( range );
    }
    for (auto& it: dataMap().user_defined) {
        it.second.setMaximumRangeX( range );
    }
}

inline
std::vector<QString> DataStreamer::appendData(PlotDataMapRef &destination)
{
    PlotDataMapRef &source = _data_map;

    std::vector<QString> added_curves;
    for (auto& it: _data_map.numeric)
    {
        const std::string& name  = it.first;
        if( it.second.size()>0 && destination.numeric.count(name) == 0)
        {
            added_curves.push_back( QString::fromStdString( name ) );
        }
    }

    for (auto& it: source.numeric)
    {
        const std::string& name  = it.first;
        auto& source_plot  = it.second;
        auto plot_with_same_name = destination.numeric.find(name);

        // this is a new plot
        if( plot_with_same_name == destination.numeric.end() )
        {
            plot_with_same_name = destination.numeric.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(name),
                        std::forward_as_tuple(name)
                        ).first;
        }
        auto& destination_plot = plot_with_same_name->second;
        for (size_t i=0; i< source_plot.size(); i++)
        {
            destination_plot.pushBack( source_plot.at(i) );
        }
        source_plot.clear();
    }

    for (auto& it: source.user_defined)
    {
        const std::string& name  = it.first;
        auto& source_plot  = it.second;
        auto plot_with_same_name = destination.user_defined.find(name);

        // this is a new plot
        if( plot_with_same_name == destination.user_defined.end() )
        {
            plot_with_same_name = destination.user_defined.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(name),
                        std::forward_as_tuple(name)
                        ).first;
        }
        auto& destination_plot = plot_with_same_name->second;
        for (size_t i=0; i< source_plot.size(); i++)
        {
            destination_plot.pushBack( source_plot.at(i) );
        }
        source_plot.clear();
    }
    return added_curves;
}

#endif

