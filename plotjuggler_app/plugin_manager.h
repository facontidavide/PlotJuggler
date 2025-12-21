#pragma once

#include <memory>
#include <QPluginLoader>
#include "PlotJuggler/dataloader_base.h"
#include "PlotJuggler/statepublisher_base.h"
#include "PlotJuggler/toolbox_base.h"
#include "PlotJuggler/datastreamer_base.h"
#include "PlotJuggler/messageparser_base.h"

namespace PJ
{
class PluginManager
{
public:
  PluginManager() = default;
  ~PluginManager() = default;
  PluginManager(const PluginManager&) = delete;
  PluginManager& operator=(const PluginManager&) = delete;

  void setEnabledPlugins(const QStringList& enabled_plugins);
  void setDisabledPlugins(const QStringList& disabled_plugins);

  void loadPluginsFromFolder(const QString& folderPath);

  const std::map<QString, DataLoaderPtr>& dataLoaders() const;
  const std::map<QString, StatePublisherPtr>& statePublishers() const;
  const std::map<QString, DataStreamerPtr>& dataStreamers() const;
  const std::map<QString, ToolboxPluginPtr>& toolboxes() const;
  const std::map<QString, ParserFactoryPtr>& parserFactories() const;

  void unloadAllPlugins();

private:
  std::unique_ptr<QPluginLoader> m_pluginLoader;
  QStringList _enabled_plugins;
  QStringList _disabled_plugins;
  bool _test_plugins_enabled = false;

  std::set<QString> _loaded_plugins;
  std::map<QString, DataLoaderPtr> _data_loader;
  std::map<QString, StatePublisherPtr> _state_publisher;
  std::map<QString, DataStreamerPtr> _data_streamer;
  std::map<QString, ToolboxPluginPtr> _toolboxes;
  std::map<QString, ParserFactoryPtr> _parser_factories;

  void loadPlugin(const QString& pluginPath);
  void loadWASM(const QString& pluginPath);
};

}  // namespace PJ
