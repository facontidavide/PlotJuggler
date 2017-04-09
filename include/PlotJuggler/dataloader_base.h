#ifndef DATALOAD_TEMPLATE_H
#define DATALOAD_TEMPLATE_H

#include <QtPlugin>
#include <QMenu>
#include <QFile>
#include <functional>
#include "PlotJuggler/plotdata.h"

enum { TIME_INDEX_NOT_DEFINED = -2 };

class DataLoader{


public:

    virtual const std::vector<const char*>& compatibleFileExtensions() const = 0;

    virtual PlotDataMap readDataFromFile(const std::string& file_name,
                                         std::string& time_index ) = 0;

    virtual const char* name() const = 0;

    virtual ~DataLoader() {}

    virtual QObject* getObject() { return nullptr;  }

    virtual bool isDebugPlugin() { return false; }

    virtual void setParentMenu(QMenu* menu) { _menu = menu; }

protected:
    QMenu* _menu;
};

QT_BEGIN_NAMESPACE

#define DataRead_iid "com.icarustechnology.PlotJuggler.DataLoader"

Q_DECLARE_INTERFACE(DataLoader, DataRead_iid)

QT_END_NAMESPACE


#endif

