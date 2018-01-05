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
	_server("plotJuggler", QWebSocketServer::NonSecureMode)	
{

	
}

DataStreamServer::~DataStreamServer()
{
	shutdown();
	_server.close();
}

bool DataStreamServer::start()
{	
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
	else
		qDebug() << "Couldn't open websocket on port " << _port;
	return _running;
}

void DataStreamServer::shutdown()
{
	socketDisconnected();
	_server.close();
	_running = false;
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
	if (pClient)
	{
		_clients.removeAll(pClient);
		pClient->deleteLater();
	}
}


