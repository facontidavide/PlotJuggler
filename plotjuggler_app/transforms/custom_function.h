#pragma once

#include <memory>
#include <string>
#include <vector>
#include <QWidget>
#include <QString>
#include <QDomDocument>
#include <QString>
#include "PlotJuggler/plotdata.h"
#include "PlotJuggler/transform_function.h"

using namespace PJ;

class CustomFunction;

typedef std::shared_ptr<CustomFunction> CustomPlotPtr;

struct SnippetData
{
  QString alias_name;
  QString global_vars;
  QString function;
  QString linked_source;
  QStringList additional_sources;
};

typedef std::map<QString, SnippetData> SnippetsMap;

SnippetData GetSnippetFromXML(const QDomElement& snippets_element);

SnippetsMap GetSnippetsFromXML(const QString& xml_text);

SnippetsMap GetSnippetsFromXML(const QDomElement& snippets_element);

QDomElement ExportSnippetToXML(const SnippetData& snippet, QDomDocument& destination_doc);

QDomElement ExportSnippets(const SnippetsMap& snippets, QDomDocument& destination_doc);

class CustomFunction : public PJ::TransformFunction
{
public:
  CustomFunction(SnippetData snippet = {});

  void setSnippet(const SnippetData& snippet);

  void reset() override;

  int numInputs() const override
  {
    return -1;
  }

  int numOutputs() const override
  {
    return 1;
  }

  QString aliasName() const
  {
    return _snippet.alias_name;
  }

  void calculate() override;

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

  const SnippetData& snippet() const;

  virtual QString language() const = 0;

  virtual void initEngine() = 0;

  void calculateAndAdd(PlotDataMapRef& src_data);

  virtual void calculatePoints(const std::vector<const PlotData*>& src_data, size_t point_index,
                               std::vector<PlotData::Point>& new_points) = 0;

protected:
  SnippetData _snippet;
  std::string _linked_plot_name;
  std::string _plot_name;

  std::vector<std::string> _used_channels;
};
