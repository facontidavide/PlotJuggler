#include "custom_function.h"

#include <limits>
#include <QFile>
#include <QMessageBox>
#include <QElapsedTimer>
#include "lua_custom_function.h"

CustomFunction::CustomFunction(const SnippetData& snippet):
  _snippet(snippet)
, _linked_plot_name(snippet.linkedSource.toStdString())
, _plot_name(snippet.name.toStdString())
{
  for( QString source: snippet.additionalSources){
    _used_channels.push_back( source.toStdString() );
  }
}

void CustomFunction::clear()
{
  // This cause a crash during streaming for reasons that are not 100% clear.
  // initEngine();
}

void CustomFunction::calculateAndAdd(PlotDataMapRef& plotData)
{
  bool newly_added = false;

  auto dst_data_it = plotData.numeric.find(_plot_name);
  if (dst_data_it == plotData.numeric.end())
  {
    dst_data_it = plotData.addNumeric(_plot_name);
    newly_added = true;
  }

  PlotData& dst_data = dst_data_it->second;
  dst_data.clear();

  try
  {
    calculate(plotData, &dst_data);
  }
  catch (...)
  {
    if (newly_added)
    {
      plotData.numeric.erase(dst_data_it);
    }
    std::rethrow_exception(std::current_exception());
  }
}

const SnippetData &CustomFunction::snippet() const
{
  return _snippet;
}

void CustomFunction::calculate(const PlotDataMapRef& plotData,
                               PlotData* dst_data)
{
  auto src_data_it = plotData.numeric.find(_linked_plot_name);
  if (src_data_it == plotData.numeric.end())
  {
    // failed! keep it empty
    return;
  }

  const PlotData& src_data = src_data_it->second;
  if (src_data.size() == 0)
  {
    return;
  }

  // clean up old data
  dst_data->setMaximumRangeX( src_data.maximumRangeX() );

  std::vector<const PlotData*> channel_data;
  channel_data.reserve(_used_channels.size());

  for (const auto& channel : _used_channels)
  {
    auto it = plotData.numeric.find(channel);
    if (it == plotData.numeric.end())
    {
      throw std::runtime_error("Invalid channel name");
    }
    const PlotData* chan_data = &(it->second);
    channel_data.push_back(chan_data);
  }

  double last_updated_stamp = -std::numeric_limits<double>::max();
  if (dst_data->size() != 0)
  {
    last_updated_stamp = dst_data->back().x;
  }

  std::vector<PlotData::Point> points;
  for (size_t i = 0; i < src_data.size(); ++i)
  {
    if (src_data.at(i).x > last_updated_stamp)
    {
      points.clear();
      calculatePoints(src_data, channel_data, i, points);

      for (PlotData::Point const &point : points) {
        dst_data->pushBack(point);
      }
    }
  }
}

const std::string& CustomFunction::name() const
{
  return _plot_name;
}

const std::string& CustomFunction::linkedPlotName() const
{
  return _linked_plot_name;
}


QDomElement CustomFunction::xmlSaveState(QDomDocument& doc) const
{
  return ExportSnippetToXML(_snippet, doc);
}

CustomPlotPtr CustomFunction::createFromXML(QDomElement& element)
{
  SnippetData snippet = GetSnippetFromXML(element);
  return std::make_unique<LuaCustomFunction>(snippet);
}

SnippetsMap GetSnippetsFromXML(const QString& xml_text)
{
  if (xml_text.isEmpty()){
    return {};
  }

  QDomDocument doc;
  QString parseErrorMsg;
  int parseErrorLine;
  if (!doc.setContent(xml_text, &parseErrorMsg, &parseErrorLine))
  {
    QMessageBox::critical(
        nullptr, "Error",
        QString("Failed to parse snippets (xml), error %1 at line %2").arg(parseErrorMsg).arg(parseErrorLine));
    return {};
  }
  else
  {
    QDomElement snippets_element = doc.documentElement();
    return GetSnippetsFromXML(snippets_element);
  }
}

SnippetsMap GetSnippetsFromXML(const QDomElement& snippets_element)
{
  SnippetsMap snippets;

  for (auto elem = snippets_element.firstChildElement("snippet");
       !elem.isNull();
       elem = elem.nextSiblingElement("snippet") )
  {
    SnippetData snippet = GetSnippetFromXML(elem);
    snippets.insert({ snippet.name, snippet });
  }
  return snippets;
}


QDomElement ExportSnippets(const SnippetsMap& snippets, QDomDocument& doc)
{
  auto snippets_root = doc.createElement("snippets");

  for (const auto& it : snippets)
  {
    const auto& snippet = it.second;
    auto element = ExportSnippetToXML(snippet, doc);
    snippets_root.appendChild(element);
  }
  return snippets_root;
}

SnippetData GetSnippetFromXML(const QDomElement &element)
{
  SnippetData snippet;
  snippet.linkedSource = element.firstChildElement("linkedSource").text().trimmed();
  snippet.name = element.attribute("name");
  snippet.globalVars = element.firstChildElement("global").text().trimmed();
  snippet.function = element.firstChildElement("function").text().trimmed();

  auto additional_el = element.firstChildElement("additionalSources");
  if( !additional_el.isNull() )
  {
    int count = 1;
    auto tag_name = QString("v%1").arg(count);
    auto source_el = additional_el.firstChildElement( tag_name );
    while( !source_el.isNull() )
    {
      snippet.additionalSources.push_back( source_el.text() );
      tag_name = QString("v%1").arg(++count);
      source_el = additional_el.firstChildElement( tag_name );
    }
  }
  return snippet;
}


QDomElement ExportSnippetToXML(const SnippetData &snippet, QDomDocument &doc)
{
  auto element = doc.createElement("snippet");

  element.setAttribute("name", snippet.name);

  auto global_el = doc.createElement("global");
  global_el.appendChild(doc.createTextNode(snippet.globalVars));
  element.appendChild(global_el);

  auto equation_el = doc.createElement("function");
  equation_el.appendChild(doc.createTextNode(snippet.function));
  element.appendChild(equation_el);

  auto linked_el = doc.createElement("linkedSource");
  linked_el.appendChild(doc.createTextNode(snippet.linkedSource));
  element.appendChild(linked_el);

  if( snippet.additionalSources.size() > 0)
  {
    auto sources_el = doc.createElement("additionalSources");

    int count = 1;
    for(QString curve_name: snippet.additionalSources)
    {
      auto tag_name = QString("v%1").arg(count++);
      auto source_el = doc.createElement(tag_name);
      source_el.appendChild(doc.createTextNode(curve_name));
      sources_el.appendChild(source_el);
    }

    element.appendChild(sources_el);
  }


  return element;
}
