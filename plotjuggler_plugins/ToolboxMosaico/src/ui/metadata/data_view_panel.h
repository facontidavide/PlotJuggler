#pragma once

#include <flight/types.hpp>
using mosaico::SequenceInfo;
using mosaico::TopicInfo;

#include <QWidget>

#include <unordered_map>
#include <string>

class QLabel;
class QTextEdit;

// Right column: shows sequence and topic metadata in a single text view.
// Sequence info stays at the top; topic metadata accumulates below
// as topics are fetched, separated by ASCII dividers.
class DataViewPanel : public QWidget
{
  Q_OBJECT

public:
  explicit DataViewPanel(QWidget* parent = nullptr);

public slots:
  void clear();
  void clearTopics();
  void showSequenceInfo(const SequenceInfo& info);
  void showTopicInfo(const TopicInfo& info);

private:
  void rebuildText();
  template <typename MapType>
  static QString formatMetadata(const MapType& metadata, const QString& indent = "  ");

  QLabel* header_ = nullptr;
  QTextEdit* text_view_ = nullptr;

  // Cached sequence info — persists across topic selections.
  QString sequence_text_;

  // Accumulated topic info — one entry per fetched topic.
  QStringList topic_texts_;
};
