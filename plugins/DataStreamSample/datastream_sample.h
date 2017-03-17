#ifndef DATASTREAM_SAMPLE_CSV_H
#define DATASTREAM_SAMPLE_CSV_H

#include <QtPlugin>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"


class  DataStreamSample: public QObject, DataStreamer
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

    virtual bool isStreamingEnabled() const override;

    virtual ~DataStreamSample();

    virtual const char* name() const override { return "DataStreamer Dummy"; }

    virtual QObject* getObject() override { return this; }

    virtual bool isDebugPlugin() override { return true; }

private:

    struct Parameters{
        double A,B,C,D;
    };

    void loop();

    PlotDataMap _plot_data;
    bool _enabled;

    std::thread _thread;

    bool _running;
    double _simulated_time;

    std::map<std::string,Parameters> _parameters;


};

#endif // DATALOAD_CSV_H
