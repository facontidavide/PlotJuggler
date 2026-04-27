/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <flight/types.hpp>
using mosaico::PullResult;
using mosaico::SequenceInfo;
using mosaico::TimeRange;
using mosaico::TopicInfo;
#include "types.h"

#include <QHash>
#include <QSet>
#include <QString>
#include <QWidget>

#include <memory>
#include <set>
#include <vector>

class QComboBox;
class QLabel;
class QPushButton;
class ElidedLabel;
class QSettings;
class QSplitter;
class QThread;
class RangeSlider;

namespace mosaico
{
class MosaicoClient;
}
using mosaico::MosaicoClient;

class DataViewPanel;
class FetchWorker;
class QueryBar;
class SequencePanel;
class TopicPanel;

// Top-level window. Owns all panels and the FetchWorker background thread.
// All cross-panel signal/slot wiring lives here; panels are unaware of
// each other.
class MainWindow : public QWidget
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

  void saveState(QSettings& settings) const;
  void restoreState(QSettings& settings);

  // Initiates a connection to the server. If explicit_connect is true,
  // connection errors will be shown as a popup; otherwise only in the status label.
  void connectToServer(bool explicit_connect);

  const std::vector<SequenceInfo>& sequences() const
  {
    return all_sequences_;
  }
  void setVisibleSequences(const std::set<std::string>& names);
  void clearVisibleSequences();
  void updateTheme(bool dark);

signals:
  void mosaicoDataReady(const QString& sequence_name, const QString& topic_name,
                        const PullResult& result);
  void mosaicoTopicStarted(const QString& sequence_name, const QString& topic_name,
                           const std::shared_ptr<arrow::Schema>& schema);
  void mosaicoTopicBatchReady(const QString& sequence_name, const QString& topic_name,
                              const std::shared_ptr<arrow::RecordBatch>& batch);
  void mosaicoTopicFinished(const QString& sequence_name, const QString& topic_name);
  void allFetchesComplete();
  // Emitted when a user-initiated cancel has finished draining the worker's
  // queue. The plugin wrapper uses this to discard any partial imported data.
  void fetchCancelled();
  void backRequested();
  void queryChanged(const QString& query, bool valid);
  void schemaReady(const Schema& schema);

private slots:
  void onSequenceSelected(const QString& sequence_name);
  void onTopicsSelected(const QString& sequence_name, const QStringList& topic_names);
  void onDataReady(const QString& sequence_name, const QString& topic_name,
                   const PullResult& result);
  void onTopicStreamStarted(const QString& sequence_name, const QString& topic_name,
                            const std::shared_ptr<arrow::Schema>& schema);
  void onTopicBatchReady(const QString& sequence_name, const QString& topic_name,
                         const std::shared_ptr<arrow::RecordBatch>& batch);
  void onTopicStreamFinished(const QString& sequence_name, const QString& topic_name);
  void onFetchError(const QString& message);
  void onTopicFetchError(const QString& topic_name, const QString& message);
  void onFetchProgress(const QString& topic_name, qint64 bytes, qint64 total_bytes,
                       bool from_network);
  void onSequencesReady(const std::vector<SequenceInfo>& sequences);
  void onTopicsReady(const QStringList& names, const std::vector<TopicInfo>& infos);
  void onTopicMetadataReady(const QString& sequence_name, const QString& topic_name,
                            const TopicInfo& info);
  void onFetchClicked();
  void onConnectClicked();
  void onRefreshClicked();
  void onQueryChanged(const QString& query, bool valid);
  void onServerSelected(int index);

private:
  void buildLayout();
  void connectSignals();
  void ensureWorkerThread();
  void requestFetchCancel();
  void finishFetchTopic(const QString& topic_name, bool success);
  void finishFetchBatch();

  // Returns the current edit text of the server combo — replaces the
  // old server_uri_input_->text() reads.
  [[nodiscard]] QString currentServerText() const;

  // Sets the current edit text — replaces server_uri_input_->setText().
  void setCurrentServerText(const QString& text);

  // Rebuild the server_uri_combo_ items from `history_` plus the pinned
  // demo seed. Preserves the current edit-field text.
  void rebuildServerCombo();

  // Normalize, dedup-promote, cap, persist, then rebuild the combo.
  // Called on successful connect. No-op if `key` is empty or equals the
  // pinned demo seed (the demo is always implicit).
  void addServerToHistory(const QString& key);

  // Returns the current URI normalized to a server key, or empty if the
  // edit text isn't a valid key.
  [[nodiscard]] QString currentServerKey() const;

  // Load `cert_path_` and `api_key_` from server_cache/<key>/, or clear
  // both if no cache entry exists for that key. Safe to call with an
  // empty key (no-op).
  void loadCachedCredentials(const QString& key);

  // Write current `cert_path_` and `api_key_` to server_cache/<key>/.
  // No-op if key is empty.
  void writeCachedCredentials(const QString& key);

  void setSequenceRange(int64_t min_ts_ns, int64_t max_ts_ns);
  int64_t sliderToNs(int pos) const;
  void onCertButtonClicked();
  // Set the status-label text, appending " (secure connection)" /
  // " (insecure connection)" according to connection_mode_. When disconnected,
  // no suffix is appended.
  void setStatus(const QString& message);

  static constexpr int kSliderSteps = 1'000'000;
  static constexpr const char* kDemoServerKey = "demo.mosaico.dev:6726";
  static constexpr int kServerHistoryCap = 20;

  std::unique_ptr<MosaicoClient> client_;

  SequencePanel* sequence_panel_ = nullptr;
  TopicPanel* topic_panel_ = nullptr;
  DataViewPanel* data_view_panel_ = nullptr;
  QueryBar* query_bar_ = nullptr;

  // Range slider row — always visible.
  RangeSlider* range_slider_ = nullptr;
  QPushButton* fetch_button_ = nullptr;
  QPushButton* refresh_button_ = nullptr;

  QSplitter* top_splitter_ = nullptr;
  QSplitter* root_splitter_ = nullptr;

  ElidedLabel* status_label_ = nullptr;
  QComboBox* server_uri_combo_ = nullptr;

  QPushButton* connect_button_ = nullptr;

  // Certificate/API-key entry button (inline in the URI row).
  QPushButton* cert_button_ = nullptr;

  // MRU list of successfully-connected servers (normalized host:port).
  // Excludes the pinned demo seed.
  QStringList server_history_;

  // TLS/auth state
  QString cert_path_;
  QString api_key_;
  // Per-URL opt-in to plaintext fallback when the TLS handshake fails and no
  // custom cert is set. Default false; persisted per server key alongside
  // cert_path_ / api_key_.
  bool allow_insecure_ = false;
  // Transient: true while a single Connect attempt has already tried TLS and
  // is now retrying plaintext. Reset on every new connect attempt.
  bool attempted_plaintext_fallback_ = false;

  // Current connection security. Set to Secure after a successful TLS
  // connect, Insecure after a plaintext fallback succeeds, None while
  // disconnected. Drives the suffix in setStatus().
  enum class ConnectionMode
  {
    None,
    Secure,
    Insecure
  };
  ConnectionMode connection_mode_ = ConnectionMode::None;

  FetchWorker* worker_ = nullptr;
  QThread* worker_thread_ = nullptr;

  // Tracks error context so we only show popups for explicit Connect clicks.
  enum class ErrorContext
  {
    None,
    AutoConnect,
    ExplicitConnect,
    Fetch
  };
  ErrorContext error_context_ = ErrorContext::None;
  // True between a user-initiated Cancel click and the moment the queued
  // topics all report back. Drives the discard-results path in onDataReady /
  // onFetchError and the "Cancelling…" label.
  bool cancelling_fetch_ = false;

  int64_t seq_min_ns_ = 0;
  int64_t seq_max_ns_ = 0;
  bool long_format_ = false;
  double lower_proportion_ = 0.0;
  double upper_proportion_ = 1.0;

  std::vector<SequenceInfo> all_sequences_;
  std::vector<TopicInfo> cached_topic_infos_;

  // Lazily-populated full TopicInfo (schema/ontology/user_metadata) keyed by
  // "sequence/topic". Filled by getTopicMetadata RPCs that fire when the user
  // selects a topic, so we never pay the round-trip for topics they don't
  // inspect. Cleared on connect/disconnect.
  QHash<QString, TopicInfo> topic_metadata_cache_;
  // Topics currently in flight to suppress duplicate fetches when the user
  // re-selects the same topic before the first response lands.
  QSet<QString> topic_metadata_in_flight_;

  QString selected_sequence_;
  QStringList selected_topics_;
  int pending_fetches_ = 0;

  // Per-fetch error accumulator. A batch can produce one error per topic,
  // and with dozens of topics the dialog-per-error flow was unusable —
  // users had to dismiss each popup before reaching the next one. Collect
  // errors here and show a single summary when the batch finishes. Keyed
  // by message so identical errors across N topics collapse to one entry
  // with a count.
  QHash<QString, int> pending_fetch_errors_;
};
