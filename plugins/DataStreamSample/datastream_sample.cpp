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
    _simulated_time(0),
    _enabled(false)
{
    QStringList  words_list;
    words_list << "siam" << "tre" << "piccoli" << "porcellin"
               << "mai" << "nessun" << "ci" << "dividera";

    foreach( const QString& name, words_list)
    {
        double A =  qrand()/(double)RAND_MAX * 6 - 3;
        double B =  qrand()/(double)RAND_MAX *3;
        double C =  qrand()/(double)RAND_MAX *3;
        double D =  qrand()/(double)RAND_MAX *2 -1;

        vect_A.push_back(A);
        vect_B.push_back(B);
        vect_C.push_back(C);
        vect_D.push_back(D);

        PlotDataPtr plot( new PlotData() );
        plot->setName( name.toStdString() );
        plot->setMaximumRangeX( 4.0 );
        _plot_data.numeric.insert( std::make_pair( name.toStdString(), plot) );
    }
}

bool DataStreamSample::start()
{
    _running = true;
    _thread = std::thread([this](){ this->loop();} );
    return true;
}

void DataStreamSample::shutdown()
{
    _running = false;
    if( _thread.joinable()) _thread.join();
}

void DataStreamSample::enableStreaming(bool enable) { _enabled = enable; }

bool DataStreamSample::isStreamingEnabled() const { return _enabled; }

DataStreamSample::~DataStreamSample()
{
    shutdown();
}



void DataStreamSample::loop()
{
    static auto prev_time = std::chrono::high_resolution_clock::now();

    _running = true;
    while( _running )
    {
        size_t index=0;
        _simulated_time += 0.01;

        PlotData::asyncPushMutex().lock();
        for (auto& it: _plot_data.numeric )
        {
            double A =  vect_A[index];
            double B =  vect_B[index];
            double C =  vect_C[index];
            double D =  vect_D[index];
            index++;

            auto& plot = it.second;
            const double t = _simulated_time;
            double y =  A*sin(B*t + C) +D*t*0.05;

            if( _enabled ){
                // IMPORTANT: don't use pushBack(), it may cause a segfault
                plot->pushBackAsynchronously( PlotData::Point( t, y ) );
            }
        }
        PlotData::asyncPushMutex().unlock();
        prev_time += std::chrono::milliseconds(10);
        std::this_thread::sleep_until ( prev_time );
    }
}
