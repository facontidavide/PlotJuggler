#ifndef DATA_STREAMER_TEMPLATE_H
#define DATA_STREAMER_TEMPLATE_H

#include <QtPlugin>
#include <QMenu>
#include <QDomDocument>
#include <mutex>
#include "PlotJuggler/plotdata.h"


class DataStreamer: public QObject{

    Q_OBJECT
public:
    DataStreamer(): _menu(NULL){}

    virtual PlotDataMap& getDataMap() = 0;

    virtual bool start() = 0;

    virtual void shutdown() = 0;

    virtual void enableStreaming(bool enable) = 0;

    virtual bool isStreamingRunning() const = 0;

    virtual ~DataStreamer() {}

    virtual const char* name() const = 0;

    virtual bool isDebugPlugin() { return false; }

    virtual void setParentMenu(QMenu* menu) { _menu = menu; }

    virtual QWidget* embeddedWidget() { return nullptr; }

    virtual QDomElement xmlSaveState(QDomDocument &doc) const { return QDomElement(); }

    virtual bool xmlLoadState(QDomElement &parent_element ) { return false; }

signals:

    void dataUpdated();

    void connectionClosed();

protected:
    QMenu* _menu;
};

QT_BEGIN_NAMESPACE

#define DataStream_iid "com.icarustechnology.PlotJuggler.DataStreamer"

Q_DECLARE_INTERFACE(DataStreamer, DataStream_iid)

QT_END_NAMESPACE


#endif

