#ifndef PJ_PLUGIN_H
#define PJ_PLUGIN_H

#include <QtPlugin>
#include <QMenu>
#include <QWidget>
#include <QDomDocument>
#include "PlotJuggler/plotdata.h"

namespace PJ {

class PlotJugglerPlugin : public QObject
{
public:
  PlotJugglerPlugin()
  {
  }

  virtual const char* name() const = 0;

  virtual bool isDebugPlugin()
  {
    return false;
  }

  virtual QWidget* optionsWidget()
  {
    return nullptr;
  }

  virtual bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
  {
    return false;
  }

  virtual bool xmlLoadState(const QDomElement& parent_element)
  {
    return false;
  }

  QDomElement xmlSaveState(QDomDocument& doc) const
  {
    QDomElement plugin_elem = doc.createElement("plugin");
    plugin_elem.setAttribute("ID", this->name());
    xmlSaveState(doc, plugin_elem);
    return plugin_elem;
  }

  virtual const std::vector<QAction*>& availableActions()
  {
    static std::vector<QAction*> empty;
    return empty;
  }
};

}

#endif  // PJ_PLUGIN_H
