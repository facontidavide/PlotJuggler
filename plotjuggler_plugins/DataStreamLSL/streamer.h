#ifndef STREAMER_H
#define STREAMER_H

#include <QObject>
#include <QThread>
#include "lsl_cpp.h"


class Streamer : public QObject
{
    Q_OBJECT
public:
    explicit Streamer(QObject *parent = nullptr);

    bool queryStream(QString name);

    std::vector<std::string> channelList() const;

signals:
    void stopped();
    void dataReceived(std::vector< std::vector<double> > *chunk, std::vector<double> *stamps);

public slots:
    void stream();
    void stop();

private:
    bool initStreamInlet();
    void setChannelNames(lsl::stream_info info);

private:
    lsl::stream_info m_streamInfo;
    bool m_running;
    std::vector<std::string> channel_list;
};

Q_DECLARE_OPAQUE_POINTER(std::vector< std::vector<int> > *)
Q_DECLARE_OPAQUE_POINTER(std::vector<double> *)

#endif // STREAMER_H
