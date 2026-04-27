/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <QtPlugin>
#include <QSettings>
#include "PlotJuggler/toolbox_base.h"

#include <arrow/type.h>
#include <flight/types.hpp>
using mosaico::PullResult;
using mosaico::SequenceInfo;
using mosaico::TopicInfo;

#include "src/query/engine.h"
#include "src/core/types.h"

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

class MainWindow;

class ToolboxMosaico : public PJ::ToolboxPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.Toolbox")
  Q_INTERFACES(PJ::ToolboxPlugin)

public:
  ToolboxMosaico();
  ~ToolboxMosaico() override;

  const char* name() const override
  {
    return "Mosaico";
  }

  void init(PJ::PlotDataMapRef& src_data, PJ::TransformsMap& transform_map) override;
  std::pair<QWidget*, WidgetType> providedWidget() const override;

public slots:
  bool onShowWidget() override;

private slots:
  void onMosaicoDataReady(const QString& sequence_name, const QString& topic_name,
                          const PullResult& result);
  void onMosaicoTopicStarted(const QString& sequence_name, const QString& topic_name,
                             const std::shared_ptr<arrow::Schema>& schema);
  void onMosaicoTopicBatchReady(const QString& sequence_name, const QString& topic_name,
                                const std::shared_ptr<arrow::RecordBatch>& batch);
  void onMosaicoTopicFinished(const QString& sequence_name, const QString& topic_name);
  void onAllFetchesComplete();
  void onSchemaReady(const Schema& schema);
  void onQueryChanged(const QString& query, bool valid);

private:
  void convertRecordBatchToSeries(const QString& sequence_name, const QString& topic_name,
                                  const PullResult& result);
  bool beginTopicImport(const QString& sequence_name, const QString& topic_name,
                        const std::shared_ptr<arrow::Schema>& schema);
  void appendRecordBatchToSeries(const QString& sequence_name, const QString& topic_name,
                                 const std::shared_ptr<arrow::RecordBatch>& batch);
  void flattenArray(const std::shared_ptr<arrow::Array>& array,
                    const std::shared_ptr<arrow::Array>& ts_array, arrow::Type::type ts_type,
                    bool ts_is_ns, int64_t num_rows, const std::string& path,
                    std::set<std::string>& created_series);

  PJ::PlotDataMapRef* plot_data_ = nullptr;
  PJ::PlotDataMapRef imported_data_;
  MainWindow* main_window_ = nullptr;
  struct TopicImportState
  {
    std::shared_ptr<arrow::Schema> schema;
    int ts_col = -1;
    arrow::Type::type ts_type = arrow::Type::NA;
    bool ts_is_ns = false;
    std::string prefix;
    std::set<std::string> created_series;
  };
  std::unordered_map<std::string, TopicImportState> topic_imports_;
  Engine engine_;
  Schema schema_;
  bool initialized_ = false;
};
