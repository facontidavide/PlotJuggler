#include "query_bar.h"
#include "../theme_utils.h"
#include "../colors.h"
#include "completer.h"

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPalette>
#include <QTextCharFormat>
#include <QVBoxLayout>

#include <QCodeEditor>
#include <QLuaHighlighter>
#include <QSyntaxStyle>

QueryBar::QueryBar(QWidget* parent) : QWidget(parent)
{
  buildUi();
  wire();

  editor_->viewport()->setMouseTracking(true);
  editor_->viewport()->installEventFilter(this);
}

void QueryBar::setSchema(const Schema& schema)
{
  schema_ = schema;
  populateKeyCombo();
}

QString QueryBar::query() const { return editor_->toPlainText(); }
void QueryBar::setQuery(const QString& text) { editor_->setPlainText(text); }

void QueryBar::updateTheme(bool dark)
{
  QFile fl(dark ? ":/resources/lua_style_dark.xml"
                : ":/resources/lua_style_light.xml");
  if (fl.open(QIODevice::ReadOnly))
  {
    auto* style = new QSyntaxStyle(this);
    if (style->load(fl.readAll()))
      editor_->setSyntaxStyle(style);
    else
      delete style;
  }
}

void QueryBar::changeEvent(QEvent* event)
{
  if (event->type() == QEvent::StyleChange)
  {
    updateTheme(isDarkTheme());
  }
  QWidget::changeEvent(event);
}

bool QueryBar::eventFilter(QObject* obj, QEvent* event)
{
  if (obj == editor_->viewport() && event->type() == QEvent::MouseMove)
  {
    auto* me = static_cast<QMouseEvent*>(event);
    auto cursor = editor_->cursorForPosition(me->pos());
    int hover_pos = cursor.position();
    if (hover_pos != last_hover_pos_)
    {
      last_hover_pos_ = hover_pos;
      if (!in_handler_)
      {
        in_handler_ = true;
        highlightGroupAt(hover_pos);
        in_handler_ = false;
      }
    }
  }
  return QWidget::eventFilter(obj, event);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void QueryBar::onTextChanged()
{
  if (in_handler_) return;
  in_handler_ = true;

  auto text = editor_->toPlainText();
  auto std_text = text.toStdString();

  auto vr = Engine::validate(std_text);
  emit queryChanged(text, vr.valid);

  if (std_text.empty())
  {
    feedback_->hide();
  }
  else if (!vr.valid)
  {
    feedback_->setText("invalid syntax");
    auto pal = feedback_->palette();
    pal.setColor(QPalette::WindowText, kErrorAccent);
    feedback_->setPalette(pal);
    feedback_->show();
  }
  else
  {
    feedback_->setText("ok");
    auto pal = feedback_->palette();
    pal.setColor(QPalette::WindowText, kSuccessGreen);
    feedback_->setPalette(pal);
    feedback_->show();
    emit filterChanged(text);
  }

  syncDropdowns();
  highlightGroupAt(last_hover_pos_);

  in_handler_ = false;
}

void QueryBar::onCursorChanged()
{
  if (in_handler_) return;
  in_handler_ = true;
  syncDropdowns();
  in_handler_ = false;
}

void QueryBar::onKeyComboChanged(int index)
{
  if (suppress_combo_signals_ || index < 0) return;
  auto key = key_combo_->itemData(index).toString();
  if (key.isEmpty()) return;

  if (ctx_.key_action == Action::Replace)
    applyEdit(ctx_.active_token.start, ctx_.active_token.end, key);
  else
    applyInsert(key);
}

void QueryBar::onOpComboChanged(int index)
{
  if (suppress_combo_signals_ || index < 0) return;
  auto op = op_combo_->currentText();
  if (op.isEmpty()) return;

  if (ctx_.op_action == Action::Replace)
    applyEdit(ctx_.active_token.start, ctx_.active_token.end, op);
  else
    applyInsert(op);
}

void QueryBar::onValComboChanged(int index)
{
  if (suppress_combo_signals_ || index < 0) return;
  auto val = val_combo_->currentText();
  if (val.isEmpty()) return;

  auto quoted = QString("\"%1\"").arg(val);
  if (ctx_.val_action == Action::Replace)
    applyEdit(ctx_.active_token.start, ctx_.active_token.end, quoted);
  else
    applyInsert(quoted);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void QueryBar::buildUi()
{
  key_combo_ = new QComboBox(this);
  key_combo_->setPlaceholderText("Key");
  key_combo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  op_combo_ = new QComboBox(this);
  op_combo_->setPlaceholderText("Op");
  op_combo_->setFixedWidth(60);
  op_combo_->setEnabled(false);
  for (const auto& op : operators())
    op_combo_->addItem(QString::fromStdString(op));

  val_combo_ = new QComboBox(this);
  val_combo_->setPlaceholderText("Value");
  val_combo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  val_combo_->setEnabled(false);

  auto* combo_row = new QHBoxLayout();
  combo_row->addWidget(key_combo_, 2);
  combo_row->addWidget(op_combo_);
  combo_row->addWidget(val_combo_, 2);

  editor_ = new QCodeEditor(this);
  editor_->setHighlighter(new QLuaHighlighter);
  editor_->setAutoParentheses(true);
  editor_->setTabReplace(true);
  editor_->setTabReplaceSize(2);
  editor_->setFrameShape(QFrame::NoFrame);

  completer_ = new MetadataCompleter(this);
  editor_->setCompleter(completer_);

  feedback_ = new QLabel(this);
  feedback_->hide();
  feedback_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(combo_row);
  layout->addWidget(editor_, 1);
  layout->addWidget(feedback_);
  setLayout(layout);
}

void QueryBar::wire()
{
  connect(editor_, &QTextEdit::textChanged, this, &QueryBar::onTextChanged);
  connect(editor_, &QTextEdit::cursorPositionChanged, this, &QueryBar::onCursorChanged);

  connect(key_combo_, QOverload<int>::of(&QComboBox::activated),
          this, &QueryBar::onKeyComboChanged);
  connect(op_combo_, QOverload<int>::of(&QComboBox::activated),
          this, &QueryBar::onOpComboChanged);
  connect(val_combo_, QOverload<int>::of(&QComboBox::activated),
          this, &QueryBar::onValComboChanged);
}

void QueryBar::populateKeyCombo()
{
  suppress_combo_signals_ = true;
  key_combo_->clear();
  for (const auto& [key, vals] : schema_)
    key_combo_->addItem(QString::fromStdString(key), QString::fromStdString(key));
  key_combo_->setCurrentIndex(-1);
  suppress_combo_signals_ = false;
}

void QueryBar::populateValCombo(const std::string& key)
{
  suppress_combo_signals_ = true;
  val_combo_->clear();
  auto it = schema_.find(key);
  if (it != schema_.end())
  {
    for (const auto& val : it->second)
      val_combo_->addItem(QString::fromStdString(val));
  }
  val_combo_->setCurrentIndex(-1);
  suppress_combo_signals_ = false;
}

void QueryBar::syncDropdowns()
{
  suppress_combo_signals_ = true;

  auto text = editor_->toPlainText().toStdString();
  auto cursor_pos = editor_->textCursor().position();

  auto comp = complete(text, static_cast<std::size_t>(cursor_pos), schema_);
  completer_->update(comp);

  ctx_ = analyze(text, cursor_pos, schema_);

  key_combo_->setEnabled(ctx_.can_pick_key());
  op_combo_->setEnabled(ctx_.can_pick_op());
  val_combo_->setEnabled(ctx_.can_pick_value());

  if (!ctx_.context_key.empty())
    setComboByData(key_combo_, ctx_.context_key);
  else
    key_combo_->setCurrentIndex(-1);

  if (ctx_.can_pick_value() && !ctx_.context_key.empty())
    populateValCombo(ctx_.context_key);

  op_combo_->setCurrentIndex(-1);

  suppress_combo_signals_ = false;
}

void QueryBar::setComboByData(QComboBox* combo, const std::string& value)
{
  auto qval = QString::fromStdString(value);
  for (int i = 0; i < combo->count(); ++i)
  {
    if (combo->itemData(i).toString() == qval)
    {
      combo->setCurrentIndex(i);
      return;
    }
  }
  combo->setCurrentIndex(-1);
}

void QueryBar::applyEdit(int from, int to, const QString& new_text)
{
  auto cursor = editor_->textCursor();
  cursor.setPosition(from);
  cursor.setPosition(to, QTextCursor::KeepAnchor);

  QString replacement;
  if (from > 0)
  {
    auto prev = editor_->document()->characterAt(from - 1);
    if (!prev.isSpace()) replacement += " ";
  }
  replacement += new_text;
  if (to < editor_->document()->characterCount() - 1)
  {
    auto next = editor_->document()->characterAt(to);
    if (!next.isSpace()) replacement += " ";
  }
  else
  {
    replacement += " ";
  }

  cursor.insertText(replacement);
  editor_->setTextCursor(cursor);
  editor_->setFocus();
}

void QueryBar::applyInsert(const QString& text)
{
  auto cursor = editor_->textCursor();
  auto pos = cursor.position();

  QString insertion;
  if (pos > 0)
  {
    auto prev = editor_->document()->characterAt(pos - 1);
    if (!prev.isSpace()) insertion += " ";
  }
  insertion += text + " ";

  cursor.insertText(insertion);
  editor_->setTextCursor(cursor);
  editor_->setFocus();
}

// ---------------------------------------------------------------------------
// Group highlighting
// ---------------------------------------------------------------------------

void QueryBar::highlightGroupAt(int pos)
{
  editor_->updateExtraSelection();
  auto extras = editor_->extraSelections();

  auto text = editor_->toPlainText();
  int len = text.size();
  if (text.isEmpty() || pos < 0 || pos > len)
  {
    editor_->setExtraSelections(extras);
    return;
  }

  int start = -1;
  int end = -1;

  int open = findOpenParen(text, pos);
  if (open >= 0)
  {
    int close = findCloseParen(text, open);
    if (close > open)
    {
      start = open;
      end = close + 1;
    }
  }

  if (start < 0)
  {
    auto bounds = findOrSegment(text, pos);
    start = bounds.first;
    end = bounds.second;
  }

  if (start < 0 || end <= start || start >= len)
  {
    editor_->setExtraSelections(extras);
    return;
  }

  if (end > len) end = len;

  QTextEdit::ExtraSelection selection;
  selection.format.setBackground(group_highlight_color_);
  selection.cursor = QTextCursor(editor_->document());
  selection.cursor.setPosition(start);
  selection.cursor.setPosition(end, QTextCursor::KeepAnchor);
  extras.append(selection);

  editor_->setExtraSelections(extras);
}

std::pair<int, int> QueryBar::findOrSegment(const QString& text, int pos)
{
  int len = text.size();
  if (len == 0) return {-1, -1};

  if (pos < 0) pos = 0;
  if (pos > len) pos = len;

  std::vector<int> or_positions;
  {
    int depth = 0;
    bool in_str = false;
    QChar qch;
    for (int i = 0; i < len; ++i)
    {
      QChar ch = text.at(i);

      if (in_str)
      {
        if (ch == '\\' && i + 1 < len) { ++i; continue; }
        if (ch == qch) in_str = false;
        continue;
      }
      if (ch == '"' || ch == '\'') { in_str = true; qch = ch; continue; }
      if (ch == '(') { ++depth; continue; }
      if (ch == ')') { if (depth > 0) --depth; continue; }

      if (depth == 0 && ch == 'o' && i + 1 < len && text.at(i + 1) == 'r')
      {
        bool left_ok = (i == 0 || (!text.at(i - 1).isLetterOrNumber() && text.at(i - 1) != '_'));
        bool right_ok = (i + 2 >= len || (!text.at(i + 2).isLetterOrNumber() && text.at(i + 2) != '_'));
        if (left_ok && right_ok)
        {
          or_positions.push_back(i);
          ++i;
        }
      }
    }
  }

  if (or_positions.empty())
    return trimSegment(text, 0, len);

  int seg_start = 0;
  int seg_end = len;

  for (int or_pos : or_positions)
  {
    if (pos <= or_pos)
    {
      seg_end = or_pos;
      break;
    }
    else
    {
      seg_start = or_pos + 2;
    }
  }

  return trimSegment(text, seg_start, seg_end);
}

std::pair<int, int> QueryBar::trimSegment(const QString& text, int start, int end)
{
  int len = text.size();
  while (start < end && start < len && text.at(start).isSpace()) ++start;
  while (end > start && end <= len && text.at(end - 1).isSpace()) --end;
  if (start >= end) return {-1, -1};
  return {start, end};
}

int QueryBar::findOpenParen(const QString& text, int pos)
{
  int depth = 0;
  for (int i = pos - 1; i >= 0; --i)
  {
    QChar ch = text.at(i);
    if (ch == ')') ++depth;
    else if (ch == '(')
    {
      if (depth == 0) return i;
      --depth;
    }
  }
  return -1;
}

int QueryBar::findCloseParen(const QString& text, int open)
{
  int depth = 0;
  int len = text.size();
  for (int i = open; i < len; ++i)
  {
    QChar ch = text.at(i);
    if (ch == '(') ++depth;
    else if (ch == ')')
    {
      --depth;
      if (depth == 0) return i;
    }
  }
  return -1;
}
