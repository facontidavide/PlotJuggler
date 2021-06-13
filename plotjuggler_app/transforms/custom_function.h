#pragma once

#include <memory>
#include <string>
#include <vector>
#include <QWidget>
#include <QString>
#include <QDomDocument>
#include <QString>
#include "PlotJuggler/plotdata.h"

using namespace PJ;

class CustomFunction;

typedef std::shared_ptr<CustomFunction> CustomPlotPtr;
typedef std::unordered_map<std::string, CustomPlotPtr> CustomPlotMap;

struct SnippetData
{
  QString name;
  QString globalVars;
  QString function;
  QString linkedSource;
  QStringList additionalSources;
};

typedef std::map<QString, SnippetData> SnippetsMap;

SnippetData GetSnippetFromXML(const QDomElement& snippets_element);

SnippetsMap GetSnippetsFromXML(const QString& xml_text);

SnippetsMap GetSnippetsFromXML(const QDomElement& snippets_element);

QDomElement ExportSnippetToXML(const SnippetData& snippet, QDomDocument& destination_doc);

QDomElement ExportSnippets(const SnippetsMap& snippets, QDomDocument& destination_doc);


class CustomFunction
{
public:

  CustomFunction(const SnippetData& snippet);

  void clear();

  void calculateAndAdd(PlotDataMapRef& plotData);

  const SnippetData& snippet() const;

  const std::string& name() const;

  const std::string& linkedPlotName() const;

  QDomElement xmlSaveState(QDomDocument& doc) const;

  static CustomPlotPtr createFromXML(QDomElement& element);

  void calculate(const PlotDataMapRef& plotData, PlotData* dst_data);

  virtual QString language() const = 0;

  virtual void initEngine() = 0;

  virtual void calculatePoints(const PlotData& src_data,
                               const std::vector<const PlotData*>& channels_data,
                               size_t point_index,
                               std::vector<PlotData::Point> &new_points) = 0;

protected:
  const SnippetData _snippet;
  const std::string _linked_plot_name;
  const std::string _plot_name;

  std::vector<std::string> _used_channels;
};
