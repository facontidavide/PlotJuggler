#include "completer.h"

#include <QStringListModel>

MetadataCompleter::MetadataCompleter(QObject* parent) : QCompleter(parent)
{
  model_ = new QStringListModel(this);
  setModel(model_);
  setCompletionColumn(0);
  setModelSorting(QCompleter::CaseInsensitivelySortedModel);
  setCaseSensitivity(Qt::CaseInsensitive);
  setWrapAround(true);
  setFilterMode(Qt::MatchStartsWith);
}

void MetadataCompleter::update(const Completions& completions)
{
  QStringList list;
  for (const auto& s : completions.suggestions)
  {
    if (completions.expect == Expect::Value)
      list.append(QString("\"%1\"").arg(QString::fromStdString(s)));
    else
      list.append(QString::fromStdString(s));
  }
  list.sort(Qt::CaseInsensitive);
  model_->setStringList(list);
}
