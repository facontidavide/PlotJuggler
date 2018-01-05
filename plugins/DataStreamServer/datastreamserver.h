/*DataStreamServer PlotJuggler  Plugin license(Faircode)

Copyright(C) 2018 Philippe Gauthier - ISIR - UPMC
Permission is hereby granted to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies("Use") of the Software, and to permit persons to whom the Software is furnished to do so.
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef DATASTREAMSERVER_H
#define DATASTREAMSERVER_H

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

#endif // DATASTREAMSERVER_H
