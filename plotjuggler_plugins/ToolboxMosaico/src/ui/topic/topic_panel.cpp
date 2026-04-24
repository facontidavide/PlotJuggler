#include "topic_panel.h"

#include <algorithm>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

TopicPanel::TopicPanel(QWidget* parent) : QWidget(parent)
{
  header_ = new QLabel("Topics", this);
  auto header_font = header_->font();
  header_font.setBold(true);
  header_->setFont(header_font);

  filter_ = new QLineEdit(this);
  filter_->setPlaceholderText("Filter…");
  connect(filter_, &QLineEdit::textChanged, this, &TopicPanel::applyFilter);

  regex_btn_ = new QPushButton(".*", this);
  regex_btn_->setCheckable(true);
  regex_btn_->setToolTip("Use regular expression");
  regex_btn_->setFixedSize(24, 24);
  auto regex_font = regex_btn_->font();
  regex_font.setBold(true);
  regex_btn_->setFont(regex_font);
  regex_btn_->setStyleSheet("QPushButton { padding: 0px; }");
  connect(regex_btn_, &QPushButton::toggled, this, &TopicPanel::applyFilter);

  auto* filter_row = new QHBoxLayout();
  filter_row->addWidget(filter_);
  filter_row->addWidget(regex_btn_);

  list_ = new QListWidget(this);
  list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  connect(list_, &QListWidget::itemSelectionChanged, this, &TopicPanel::onSelectionChanged);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(header_);
  layout->addLayout(filter_row);
  layout->addWidget(list_);
  setLayout(layout);
}

void TopicPanel::setCurrentSequence(const QString& sequence_name)
{
  current_sequence_ = sequence_name;
}

void TopicPanel::populateTopics(const QStringList& names)
{
  list_->clear();
  // Sort client-side: the server's topic iteration order is not stable
  // (endpoints come from concurrent chunk queries), so the list would
  // otherwise visibly reshuffle on every re-fetch.
  QStringList sorted = names;
  std::sort(sorted.begin(), sorted.end());
  all_topics_ = sorted;
  if (sorted.isEmpty())
  {
    header_->setText("Topics (none — click a topic manually)");
    return;
  }
  header_->setText(QString("Topics (%1)").arg(sorted.size()));
  list_->addItems(sorted);
  applyFilter();
}

void TopicPanel::setLoading(bool loading)
{
  header_->setText(loading ? "Topics (loading…)" : "Topics");
  list_->setEnabled(!loading);
}

void TopicPanel::clear()
{
  list_->clear();
  all_topics_.clear();
  header_->setText("Topics");
  current_sequence_.clear();
}

void TopicPanel::onSelectionChanged()
{
  if (current_sequence_.isEmpty())
  {
    return;
  }
  QStringList selected;
  for (auto* item : list_->selectedItems())
  {
    selected.append(item->text());
  }
  emit topicsSelected(current_sequence_, selected);
}


void TopicPanel::applyFilter()
{
  const QString pattern = filter_->text();
  const bool use_regex = regex_btn_->isChecked();

  QRegularExpression re;
  if (use_regex)
  {
    re.setPattern(pattern);
    re.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    if (!re.isValid())
    {
      re.setPattern(QRegularExpression::escape(pattern));
    }
  }

  for (int i = 0; i < list_->count(); ++i)
  {
    auto* item = list_->item(i);
    const bool matches = use_regex ? re.match(item->text()).hasMatch() :
                                     item->text().contains(pattern, Qt::CaseInsensitive);
    item->setHidden(!matches);
  }
}
