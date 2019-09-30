#ifndef PLOTJUGGLER_CONTROLLERDATASTREAM_H
#define PLOTJUGGLER_CONTROLLERDATASTREAM_H

#include <QtPlugin>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"
#include <map>
#include <QStringList>
#include <cstring>
#include <fstream>
#include "ControllerStreamUARTDecoder.h"



class  ControllerStream: public DataStreamer
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "com.icarustechnology.PlotJuggler.ControllerStream")
	Q_INTERFACES(DataStreamer)

public:

	ControllerStream();

	virtual bool start(QStringList*) override;

	virtual void shutdown() override;

	virtual bool isRunning() const override;

	virtual ~ControllerStream();

	virtual const char* name() const override { return PLUGIN_NAME; }

	virtual bool isDebugPlugin() override { return false; }

	virtual bool xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const override;

	virtual bool xmlLoadState(const QDomElement &parent_element ) override;

private:

	void loop();
	bool openPort();
	void pushSingleCycle();
	double getValueFromName(const  std::string &name, StreamSample &sample);

	std::thread _thread;
	bool _running;
	int fd;
	ControllerStreamUartDecoder uartDecoder;
};

#endif //PLOTJUGGLER_CONTROLLERDATASTREAM_H
