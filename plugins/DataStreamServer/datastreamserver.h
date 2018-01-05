#ifndef DATASTREAM_SAMPLE_CSV_H
#define DATASTREAM_SAMPLE_CSV_H

#include <QWebSocketServer>
#include <QWebSocket>
#include <QList>

#include <QtPlugin>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"


class  DataStreamServer: public QObject, DataStreamer
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "fr.upmc.isir.stech.plotjugglerplugins.DataStreamerServer")
    Q_INTERFACES(DataStreamer)

public:

    DataStreamServer();

	virtual bool start() override;

	virtual void shutdown() override;

    virtual PlotDataMap& getDataMap() override { return _plot_data; }

    virtual void enableStreaming(bool enable) override { _enabled = enable; }

    virtual bool isStreamingEnabled() const override { return _enabled; }

    virtual ~DataStreamServer();

    virtual const char* name() const override { return "DataStreamServer"; }

    virtual bool isDebugPlugin() override { return false; }

private:
	quint16 _port;
	QList<QWebSocket *> _clients;
	QWebSocketServer _server;    

    PlotDataMap _plot_data;
    bool _enabled;
    bool _running;

private slots:
	void onNewConnection();	
    void processMessage(QString message);
    void socketDisconnected();
};

#endif // DATALOAD_CSV_H
