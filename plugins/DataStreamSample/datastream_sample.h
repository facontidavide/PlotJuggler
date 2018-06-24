#ifndef DATASTREAM_SAMPLE_CSV_H
#define DATASTREAM_SAMPLE_CSV_H

#include <QtPlugin>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"


class  DataStreamSample: public DataStreamer
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.icarustechnology.PlotJuggler.DataStreamer" "../datastreamer.json")
    Q_INTERFACES(DataStreamer)

public:

    DataStreamSample();

    virtual PlotDataMap& getDataMap() override { return _plot_data; }

    virtual bool start() override;

    virtual void shutdown() override;

    virtual void enableStreaming(bool enable) override;

    virtual bool isStreamingRunning() const override;

    virtual ~DataStreamSample();

    virtual const char* name() const override { return "DataStreamer Dummy"; }

    virtual bool isDebugPlugin() override { return true; }

    virtual QDomElement xmlSaveState(QDomDocument &doc) const override;

    virtual bool xmlLoadState(QDomElement &parent_element ) override;

private:

    struct Parameters{
        double A,B,C,D;
    };

    void loop();

    PlotDataMap _plot_data;
    bool _enabled;

    std::thread _thread;

    bool _running;

    std::map<std::string,Parameters> _parameters;

    void pushSingleCycle();
};

#endif // DATALOAD_CSV_H
