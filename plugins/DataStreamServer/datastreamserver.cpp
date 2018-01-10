/*DataStreamServer PlotJuggler  Plugin license(Faircode)

Copyright(C) 2018 Philippe Gauthier - ISIR - UPMC
Permission is hereby granted to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies("Use") of the Software, and to permit persons to whom the Software is furnished to do so.
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "datastreamserver.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <thread>
#include <mutex>
#include <thread>
#include <math.h>
#include <QWebSocket>
#include <QInputDialog>

DataStreamServer::DataStreamServer() :
	_enabled(false),
	_running(false),
	_server("plotJuggler", QWebSocketServer::NonSecureMode)	
{

	
}

DataStreamServer::~DataStreamServer()
{
	shutdown();
}

bool DataStreamServer::start()
{		
	if (!_running) {
		bool ok;
		_port = QInputDialog::getInt(nullptr, tr(""),
			tr("On whish port should the server listen to:"), 6666, 1111, 65535, 1, &ok);
		if (ok && _server.listen(QHostAddress::Any, _port))
		{
			qDebug() << "Websocket listening on port" << _port;
			connect(&_server, &QWebSocketServer::newConnection,
				this, &DataStreamServer::onNewConnection);
			_running = true;
		}
		else {
			qDebug() << "Couldn't open websocket on port " << _port;
			_running = false;
		}
	}
	else {
		qDebug() << "Server already running on port " << _port;
		QMessageBox::information(nullptr,"Info",QString("Server already running on port: %1").arg(_port));		
	}
	return _running;
}

void DataStreamServer::shutdown()
{
	if (_running) {
		socketDisconnected();
		_server.close();
		_running = false;
	}
	else
		qDebug() << "Nothing to shutdown";
}

void DataStreamServer::onNewConnection()
{
	qDebug() << "DataStreamServer: onNewConnection";
	QWebSocket *pSocket = _server.nextPendingConnection();
	connect(pSocket, &QWebSocket::textMessageReceived, this, &DataStreamServer::processMessage);	
	connect(pSocket, &QWebSocket::disconnected, this, &DataStreamServer::socketDisconnected);

	_clients << pSocket;
}


void DataStreamServer::processMessage(QString message)
{
	//qDebug() << "DataStreamServer: processMessage: "<< message;
	QStringList lst = message.split(':');
	if (lst.size() == 3) {
		QString key = lst.at(0);
		double time = lst.at(1).toDouble();
		double value = lst.at(2).toDouble();

		auto& map = _plot_data.numeric;

		const std::string name_str = key.toStdString();
		auto plotIt = map.find(name_str);
		
		if (plotIt == map.end()) {
			//qDebug() << "Creating plot: " << key << '\t' << time << ":" << value;

			PlotDataPtr plot(new PlotData(name_str.c_str()));

			map.insert(std::make_pair(name_str, plot));
		}
		else
			;// qDebug() << "Using plot: " << key << '\t' << time << ":" << value;
		map[name_str]->pushBackAsynchronously(PlotData::Point(time, value));
	}	
}

void DataStreamServer::socketDisconnected()
{
	qDebug() << "DataStreamServer: socketDisconnected";
	QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
	if (pClient){
		disconnect(pClient, &QWebSocket::textMessageReceived, this, &DataStreamServer::processMessage);
		disconnect(pClient, &QWebSocket::disconnected, this, &DataStreamServer::socketDisconnected);

		_clients.removeAll(pClient);
		pClient->deleteLater();
	}
}


