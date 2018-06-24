#include "datastream_sample.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <thread>
#include <mutex>
#include <chrono>
#include <thread>
#include <math.h>

DataStreamSample::DataStreamSample():
    _enabled(false)
{
    QStringList  words_list;
    words_list << "siam" << "tre" << "piccoli" << "porcellin"
               << "mai" << "nessun" << "ci" << "dividera" << "_sin" << "_cos";

    int N = words_list.size();

    foreach( const QString& name, words_list)
    {
        DataStreamSample::Parameters param;
        if(name == "_sin"){
            param.A =  2;
            param.B =  1;
            param.C =  0;
            param.D =  0;
        }
        else if (name == "_cos"){
            param.A =  2;
            param.B =  1;
            param.C =  1.5708;
            param.D =  0;
        }
        else{
            param.A =  qrand()/(double)RAND_MAX * 6 - 3;
            param.B =  qrand()/(double)RAND_MAX *3;
            param.C =  qrand()/(double)RAND_MAX *3;
            param.D =  qrand()/(double)RAND_MAX *2 -1;
        }

        const std::string name_str = name.toStdString();
        PlotDataPtr plot( new PlotData(name_str.c_str()) );

        _plot_data.numeric.insert( std::make_pair( name_str, plot) );
        _parameters.insert( std::make_pair( name_str, param) );
    }
}

bool DataStreamSample::start()
{
    _running = true;
    pushSingleCycle();
    _thread = std::thread([this](){ this->loop();} );
    return true;
}

void DataStreamSample::shutdown()
{
    _running = false;
    if( _thread.joinable()) _thread.join();
}

void DataStreamSample::enableStreaming(bool enable) { _enabled = enable; }

bool DataStreamSample::isStreamingRunning() const { return _running; }

DataStreamSample::~DataStreamSample()
{
    shutdown();
}

QDomElement DataStreamSample::xmlSaveState(QDomDocument &doc) const
{
    return QDomElement();
}

bool DataStreamSample::xmlLoadState(QDomElement &parent_element)
{
    return false;
}

void DataStreamSample::pushSingleCycle()
{
    using namespace std::chrono;
    static std::chrono::high_resolution_clock::time_point initial_time = high_resolution_clock::now();
    const double offset = duration_cast< duration<double>>( initial_time.time_since_epoch() ).count() ;

    auto now =  high_resolution_clock::now();
    for (auto& it: _plot_data.numeric )
    {
        auto par = _parameters[it.first];

        auto& plot = it.second;
        const double t = duration_cast< duration<double>>( now - initial_time ).count() ;
        double y =  par.A*sin(par.B*t + par.C) + par.D*t*0.05;

        // IMPORTANT: don't use pushBack(), it may cause a segfault
        plot->pushBackAsynchronously( PlotData::Point( t + offset, y ) );
    }
}

void DataStreamSample::loop()
{
    _running = true;
    while( _running )
    {
        if( _enabled ){
            pushSingleCycle();
        }
        std::this_thread::sleep_for ( std::chrono::milliseconds(10) );
    }
}
