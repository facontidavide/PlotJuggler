#pragma once

#include <QWidget>

#include "complete.h"
#include "edit.h"
#include "engine.h"

class QCodeEditor;
class QComboBox;
class QLabel;
class QSyntaxStyle;

class MetadataCompleter;

class QueryBar : public QWidget
{
  Q_OBJECT

public:
  explicit QueryBar(QWidget* parent = nullptr);

  void setSchema(const Schema& schema);

  [[nodiscard]] QString query() const;
  void setQuery(const QString& text);

  void updateTheme(bool dark);

signals:
  void filterChanged(const QString& query);
  void queryChanged(const QString& query, bool valid);

protected:
  void changeEvent(QEvent* event) override;
  bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
  void onTextChanged();
  void onCursorChanged();
  void onKeyComboChanged(int index);
  void onOpComboChanged(int index);
  void onValComboChanged(int index);

private:
  void buildUi();
  void wire();
  void populateKeyCombo();
  void populateValCombo(const std::string& key);
  void syncDropdowns();
  void setComboByData(QComboBox* combo, const std::string& value);
  void applyEdit(int from, int to, const QString& new_text);
  void applyInsert(const QString& text);
  void highlightGroupAt(int pos);

  [[nodiscard]] static std::pair<int, int> findOrSegment(const QString& text, int pos);
  [[nodiscard]] static std::pair<int, int> trimSegment(const QString& text, int start, int end);
  [[nodiscard]] static int findOpenParen(const QString& text, int pos);
  [[nodiscard]] static int findCloseParen(const QString& text, int open);

  QCodeEditor* editor_ = nullptr;
  MetadataCompleter* completer_ = nullptr;
  QLabel* feedback_ = nullptr;
  QComboBox* key_combo_ = nullptr;
  QComboBox* op_combo_ = nullptr;
  QComboBox* val_combo_ = nullptr;

  Schema schema_;
  CursorContext ctx_;
  bool suppress_combo_signals_ = false;
  bool in_handler_ = false;
  int last_hover_pos_ = -1;
  QColor group_highlight_color_ = QColor(200, 220, 255, 100);
};
