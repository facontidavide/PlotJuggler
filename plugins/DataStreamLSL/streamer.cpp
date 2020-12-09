#include "streamer.h"
#include <QDebug>

Streamer::Streamer(QObject *parent) : QObject(parent)
{
    m_running = false;
}

bool Streamer::queryStream(QString name)
{
    qInfo() << "Now resolving streams..." << name;
    std::vector<lsl::stream_info> resolved_streams = lsl::resolve_stream("source_id", name.toStdString());

    if(resolved_streams.empty()) {
        qInfo() << "Cannot Resolve stream " << name;
        return false;
    }

    m_streamInfo = resolved_streams.front();

    return true;
}

void Streamer::stream()
{
    lsl::stream_inlet *inlet = new lsl::stream_inlet(m_streamInfo);

    setChannelNames(inlet->info());
    m_running = true;

    while (!QThread::currentThread()->isInterruptionRequested()) {
        std::vector< std::vector<double> > chunk;
        std::vector<double> stamps;
        inlet->pull_chunk(chunk, stamps);

        emit dataReceived(&chunk, &stamps);

        QThread::currentThread()->msleep(50);
    }

    qDebug() << "Thread stopped";
    emit stopped();
}

void Streamer::stop()
{
    m_running = false;
}

void Streamer::setChannelNames(lsl::stream_info info)
{
    lsl::xml_element ch = info.desc().child("channels").child("channel");
    std::string stream_prefix = info.source_id()+ "/" +info.type();

    for (int i = 0; i < info.channel_count(); ++i)
    {
        std::string ch_name = std::string(ch.child_value("label"));
        if(ch_name.empty())
            ch_name = "channel_" + std::to_string(i);
        channel_list.push_back( stream_prefix + "/" + ch_name);
        ch = ch.next_sibling();
    }
}

std::vector<std::string> Streamer::channelList() const
{
    return channel_list;
}
