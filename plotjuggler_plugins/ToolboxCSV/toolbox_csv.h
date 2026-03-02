#pragma once

#include <string>
#include <vector>

#include <QtPlugin>
#include "PlotJuggler/toolbox_base.h"
#include "PlotJuggler/plotwidget_base.h"
#include "toolbox_ui.h"

// Toolbox plugin to export selected PlotJuggler topics to CSV (extensible to other formats)
class ToolboxCSV : public PJ::ToolboxPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.Toolbox")
  Q_INTERFACES(PJ::ToolboxPlugin)

public:
  ToolboxCSV();

  ~ToolboxCSV() override;

  // Plugin name shown in PlotJuggler
  const char* name() const override
  {
    return "CSV data saver";
  }

  // Called once to provide access to plot data and transforms
  void init(PJ::PlotDataMapRef& src_data, PJ::TransformsMap& transform_map) override;

  // Returns the widget exposed by this toolbox
  std::pair<QWidget*, WidgetType> providedWidget() const override;

public slots:

  // Invoked when the toolbox becomes visible
  bool onShowWidget() override;

private slots:

  void onClosed();
  void onExportSingleFile(bool is_csv, QString filename);
  void onExportMultipleFiles(bool is_csv, QDir dir, QString prefix);

private:
  ToolBoxUI _ui;

  // References to PlotJuggler data structures
  PJ::PlotDataMapRef* _plot_data = nullptr;
  PJ::TransformsMap* _transforms = nullptr;

  // Compute global time range across selected topics
  bool getTimeRange(double& tmin, double& tmax) const;

  void updateTimeRange();

  // Generic table representation for exporters (time + N columns)
  struct ExportTable
  {
    std::vector<std::string> names;               // column names
    std::vector<double> time;                     // time axis
    std::vector<std::vector<double>> cols;        // per-topic values
    std::vector<std::vector<uint8_t>> has_value;  // Values checker
  };

  // Tolerance helper
  static double estimateMinDt(const PJ::PlotData& plot, size_t start_idx, double t_end);

  // Build a time-aligned export table from selected topics
  ExportTable buildExportTable(const std::vector<std::string>& topics, double t_start,
                               double t_end) const;

  bool serializeCSV(const ExportTable& table, const QString& path);
  bool serializeParquet(const ExportTable& table, const QString& path);

  // Format-aware serialization dispatch
  bool serializeTable(const ExportTable& table, const QString& path, bool is_csv);
};
