#pragma once

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QWidget>

class QListWidget;

// Center column: displays topic names for the selected sequence.
// Emits topicsSelected when the user changes the row selection.
class TopicPanel : public QWidget
{
  Q_OBJECT

public:
  explicit TopicPanel(QWidget* parent = nullptr);

public slots:
  void populateTopics(const QStringList& names);
  void setCurrentSequence(const QString& sequence_name);
  void setLoading(bool loading);
  void clear();

signals:
  // Carries both sequence and topic list so MainWindow can route the batch
  // through fetchDataMulti.
  void topicsSelected(const QString& sequence_name, const QStringList& topic_names);

private slots:
  void onSelectionChanged();
  void applyFilter();

private:
  QString current_sequence_;
  QLabel* header_ = nullptr;
  QLineEdit* filter_ = nullptr;
  QPushButton* regex_btn_ = nullptr;
  QListWidget* list_ = nullptr;
  QStringList all_topics_;
};
