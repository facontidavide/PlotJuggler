#include "ControllerDataStream.h"
#include <QFile>
#include <QMessageBox>
#include <thread>
#include <mutex>
#include <chrono>
#include <fcntl.h>   /* File Control Definitions           */
#include <termios.h> /* POSIX Terminal Control Definitions */
#include <unistd.h>  /* UNIX Standard Definitions      */
#include <cstdint>
#include <QInputDialog>
#include <QStringList>
#include <QDir>
#include <QMessageBox>

#define ASSERV_FREQ 500.0

ControllerStream::ControllerStream():_running(false),fd(-1)
{
	QStringList  words_list;
	words_list
			<< "timestamp"
			<<"currentPhaseA"
			<<"currentPhaseB"
			<<"currentPhaseC"
			<<"magnitude"
			<<"vBatMotor"
			<<"theta"
			<<"speed (RPM)"
			<<"speed (kmh)"
			<<"motorTemp"
			<<"torque"
			;



	foreach( const QString& name, words_list)
	dataMap().addNumeric(name.toStdString());
}


bool ControllerStream::openPort()
{
	bool ok;

	QDir devDir("/dev/");
	QStringList ttyListAbsolutePath;

	QFileInfoList list = devDir.entryInfoList(QStringList() << "ttyUSB*" ,QDir::System);
	for (int i = 0; i < list.size(); i++)
		ttyListAbsolutePath << list.at(i).absoluteFilePath();

	QString device = QInputDialog::getItem(nullptr,
										   tr("Which tty use ?"),
										   tr("/dev/tty?"),
										   ttyListAbsolutePath,
										   0, false, &ok);

	std::string ttyName = device.toStdString();


	if(!ok)
		return false;

	fd = open(ttyName.c_str(),O_RDWR | O_NOCTTY);
	if( fd == -1)
	{
		printf("Unable to open %s\n",ttyName.c_str());
		return false;
	}
	else
	{
		printf("Port %s opened\n",ttyName.c_str());
		return true;
	}
}

bool ControllerStream::start(QStringList*)
{
	bool ok = openPort();
	bool tryAgain = !ok;
	while(tryAgain)
	{
		QMessageBox::StandardButton reply =
				QMessageBox::question(nullptr, "Unable to open port", "Unable to open port\nTry again?",
									  QMessageBox::Yes|QMessageBox::No);
		if (reply == QMessageBox::No)
		{
			tryAgain = false;
		}
		else
		{
			ok = openPort();
			tryAgain = !ok;
		}
	}

	if(ok)
	{

		// Insert dummy Point. it seems that there's a regression on plotjuggler, this dirty hack seems now necessary
		for (auto& it: dataMap().numeric )
		{
			auto& plot = it.second;
			plot.pushBack( PlotData::Point( 0, 0 ) );
		}
		_running = true;
		_thread = std::thread([this](){ this->loop();} );
		return true;
	}
	else
	{
		return false;
	}
}

void ControllerStream::shutdown()
{
	_running = false;

	if(fd != -1)
		close(fd);

	if( _thread.joinable()) _thread.join();
}

bool ControllerStream::isRunning() const { return _running; }

ControllerStream::~ControllerStream()
{
	shutdown();
}

void ControllerStream::pushSingleCycle()
{
	StreamSample sample;
	while( uartDecoder.getDecodedSample(&sample) )
	{
		std::lock_guard<std::mutex> lock( mutex() );

		double timestamp = double(sample.timestamp) * 1.0/ASSERV_FREQ;

		for (auto& it: dataMap().numeric )
		{
			double value = getValueFromName(it.first, sample);
			if(value == NAN)
				value = 0;

			auto& plot = it.second;
			plot.pushBack( PlotData::Point( timestamp, value ) );
		}
	}
}

void ControllerStream::loop()
{
	_running = true;
	while( _running  && fd != -1)
	{
		uint8_t read_buffer[sizeof(StreamSample)+4];
		unsigned int  bytes_read = 0;
		bytes_read = read(fd,read_buffer,sizeof(read_buffer));

		if(bytes_read > 0)
			uartDecoder.processBytes(read_buffer,bytes_read );

		pushSingleCycle();
		std::this_thread::sleep_for( std::chrono::microseconds(40) );
	}

}

double ControllerStream::getValueFromName(const  std::string &name, StreamSample &sample)
{
	double value = 0;

	if( name == "timestamp")
		value = sample.timestamp;
	else if( name == "currentPhaseA")
		value = float(sample.currentPhaseA) * 1./1000.;
	else if( name == "currentPhaseB")
		value = float(sample.currentPhaseB) * 1./1000.;
	else if( name == "currentPhaseC")
		value = float(sample.currentPhaseC) * 1./1000.;
	else if( name == "magnitude")
		value = sample.magnitude;
	else if( name == "vBatMotor")
		value = sample.vBatMotor;
	else if( name == "theta")
		value = sample.theta;
	else if( name == "speed (RPM)")
		value = sample.speed;
	else if( name == "speed (kmh)")
		value = float(sample.speed) * 60 * 0.00207; //perimeter of the whell times nb of min in 1 hour
	else if( name == "motorTemp" )
		value = sample.motorTemp;
	else if( name == "torque" )
		value = sample.torque;

	return value;
}



bool ControllerStream::xmlSaveState(QDomDocument &doc, QDomElement &parent_element) const
{
	return true;
}

bool ControllerStream::xmlLoadState(const QDomElement &parent_element)
{
	return false;
}