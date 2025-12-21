#include "plugin_manager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

#ifdef WASM_RUNTIME_ENABLED
#include "wasm_runtime.hpp"
#include "wasm_parser.hpp"
#endif

namespace PJ
{

void PluginManager::loadPluginsFromFolder(const QString& folderPath)
{
  qDebug() << "Loading compatible plugins from directory: " << folderPath;
  QDir pluginsDir(folderPath);
  if (!pluginsDir.exists())
  {
    qWarning() << "Plugin folder does not exist: " << folderPath;
    return;
  }

  for (const QString& filename : pluginsDir.entryList(QDir::Files))
  {
    QFileInfo fileInfo(filename);
    QString pluginPath = pluginsDir.absoluteFilePath(filename);
    if (_loaded_plugins.find(filename) != _loaded_plugins.end())
    {
      continue;
    }
    if (fileInfo.suffix() == "so" || fileInfo.suffix() == "dll" || fileInfo.suffix() == "dylib")
    {
      loadPlugin(pluginPath);
    }
    if (fileInfo.suffix() == "wasm")
    {
      loadWASM(pluginPath);
    }
  }
}

void PluginManager::loadPlugin(const QString& filename)
{
  QPluginLoader pluginLoader(filename);
  QObject* plugin = nullptr;
  try
  {
    plugin = pluginLoader.instance();
  }
  catch (std::runtime_error& err)
  {
    qDebug() << QString("%1: skipping, because it threw the following exception: %2")
                    .arg(filename, err.what());
    return;
  }
  if (!plugin && pluginLoader.errorString().contains("is not an ELF object") == false)
  {
    qDebug() << filename << ": " << pluginLoader.errorString();
    return;
  }
  if (plugin && dynamic_cast<PlotJugglerPlugin*>(plugin))
  {
    DataLoader* loader = qobject_cast<DataLoader*>(plugin);
    StatePublisher* publisher = qobject_cast<StatePublisher*>(plugin);
    DataStreamer* streamer = qobject_cast<DataStreamer*>(plugin);
    ParserFactoryPlugin* message_parser = qobject_cast<ParserFactoryPlugin*>(plugin);
    ToolboxPlugin* toolbox = qobject_cast<ToolboxPlugin*>(plugin);

    QString plugin_name;
    QString plugin_type;
    bool is_debug_plugin = dynamic_cast<PlotJugglerPlugin*>(plugin)->isDebugPlugin();

    if (loader)
    {
      plugin_name = loader->name();
      plugin_type = "DataLoader";
    }
    else if (publisher)
    {
      plugin_name = publisher->name();
      plugin_type = "StatePublisher";
    }
    else if (streamer)
    {
      plugin_name = streamer->name();
      plugin_type = "DataStreamer";
    }
    else if (message_parser)
    {
      plugin_name = message_parser->name();
      plugin_type = "MessageParser";
    }
    else if (toolbox)
    {
      plugin_name = toolbox->name();
      plugin_type = "Toolbox";
    }

    QString message = QString("%1 is a %2 plugin").arg(filename).arg(plugin_type);
    QFileInfo fileinfo(filename);

    if ((_enabled_plugins.size() > 0) && (_enabled_plugins.contains(fileinfo.baseName()) == false))
    {
      qDebug() << message << " ...skipping, because it is not explicitly enabled";
      return;
    }
    if ((_disabled_plugins.size() > 0) && (_disabled_plugins.contains(fileinfo.baseName()) == true))
    {
      qDebug() << message << " ...skipping, because it is explicitly disabled";
      return;
    }
    if (!_test_plugins_enabled && is_debug_plugin)
    {
      qDebug() << message << " ...disabled, unless option -t is used";
      return;
    }
    qDebug() << message;
    _loaded_plugins.insert(plugin_name);

    if (loader)
    {
      _data_loader.insert(std::make_pair(plugin_name, loader));
    }
    else if (publisher)
    {
      _state_publisher.insert(std::make_pair(plugin_name, publisher));
    }
    else if (streamer)
    {
      _data_streamer.insert(std::make_pair(plugin_name, streamer));
    }
    else if (message_parser)
    {
      QStringList encodings = QString(message_parser->encoding()).split(";");
      auto parser_ptr = std::shared_ptr<ParserFactoryPlugin>(message_parser);
      for (const QString& encoding : encodings)
      {
        _parser_factories.insert(std::make_pair(encoding, parser_ptr));
      }
    }
    else if (toolbox)
    {
      _toolboxes.insert(std::make_pair(plugin_name, toolbox));
    }
  }
}

void PluginManager::setEnabledPlugins(const QStringList& enabled_plugins)
{
  _enabled_plugins = enabled_plugins;
}

void PluginManager::setDisabledPlugins(const QStringList& disabled_plugins)
{
  _disabled_plugins = disabled_plugins;
}
const std::map<QString, DataLoaderPtr>& PluginManager::dataLoaders() const
{
  return _data_loader;
}
const std::map<QString, StatePublisherPtr>& PluginManager::statePublishers() const
{
  return _state_publisher;
}
const std::map<QString, DataStreamerPtr>& PluginManager::dataStreamers() const
{
  return _data_streamer;
}
const std::map<QString, ToolboxPluginPtr>& PluginManager::toolboxes() const
{
  return _toolboxes;
}
const std::map<QString, ParserFactoryPtr>& PluginManager::parserFactories() const
{
  return _parser_factories;
}

void PluginManager::unloadAllPlugins()
{
  _data_loader.clear();
  _state_publisher.clear();
  _data_streamer.clear();
  _toolboxes.clear();
  _parser_factories.clear();
  _loaded_plugins.clear();
}

void PluginManager::loadWASM(const QString& pluginPath)
{
#ifdef WASM_RUNTIME_ENABLED
  try
  {
    auto runtime = std::make_unique<WasmRuntime>(pluginPath.toStdString());
    auto manifest = QString::fromStdString(readPluginManifest(*runtime));
    // split into lines
    std::map<QString, QString> manifest_map;
    for (const auto& line : manifest.split('\n', Qt::SkipEmptyParts))
    {
      auto parts = line.split(':');
      if (parts.size() == 2)
      {
        manifest_map[parts[0].trimmed()] = parts[1].trimmed();
      }
    }

    if (manifest_map.find("plugin_type") == manifest_map.end() ||
        manifest_map.find("name") == manifest_map.end() ||
        manifest_map.find("encoding") == manifest_map.end())
    {
      qDebug() << QString("Invalid manifest in WASM plugin:");
      std::cout << manifest.toStdString() << std::endl;
      return;
    }
    const QString plugin_name = manifest_map.at("name");
    const QString plugin_type = manifest_map.at("plugin_type");
    const QString plugin_encoding = manifest_map.at("encoding");

    qDebug() << QString("%1 is a %2 WASM plugin").arg(pluginPath).arg(plugin_type);

    if (plugin_type == "MessageParser")
    {
      auto parser =
          std::make_shared<ParserFactoryWASM>(std::move(runtime), plugin_name, plugin_encoding);
      auto encoding_list = plugin_encoding.split(';', Qt::SkipEmptyParts);
      for (const auto& encoding : encoding_list)
      {
        _parser_factories.insert(std::make_pair(encoding, parser));
      }
    }
    else
    {
      qDebug() << QString("Unsupported WASM plugin type: %1").arg(plugin_type);
      std::cout << manifest.toStdString() << std::endl;
    }
  }
  catch (const std::exception& e)
  {
    qDebug() << QString("Failed to load WASM plugin %1: %2").arg(pluginPath, e.what());
  }
#else
  qDebug() << "WASM runtime support is not enabled in this build of PlotJuggler.";
#endif
}

}  // namespace PJ
