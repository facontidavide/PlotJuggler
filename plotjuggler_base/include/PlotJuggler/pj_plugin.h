/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef PJ_PLUGIN_H
#define PJ_PLUGIN_H

#include <QtPlugin>
#include <QMenu>
#include <QWidget>
#include <QDomDocument>
#include "PlotJuggler/plotdata.h"

namespace PJ
{

/**
 * @brief The PlotJugglerPlugin is the base class of all the plugins.
 */
class PlotJugglerPlugin : public QObject
{
public:
  PlotJugglerPlugin() = default;

  /// Name of the plugin type, NOT the particular instance
  virtual const char* name() const = 0;

  /// Override this to return true, if you want this plugin to be loaded only when
  /// the command line option [-t] is used.
  virtual bool isDebugPlugin()
  {
    return false;
  }

  /**
   * @brief optionsWidget pointer to a persistent widget used to
   * set the plugin options .
   */
  virtual QWidget* optionsWidget()
  {
    return nullptr;
  }

  /// Override this method to save the status of the plugin to XML
  virtual bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
  {
    return false;
  }

  /// Override this method to load the status of the plugin from XML
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

}  // namespace PJ

#endif  // PJ_PLUGIN_H
