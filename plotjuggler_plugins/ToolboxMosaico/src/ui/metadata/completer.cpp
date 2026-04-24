/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

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
    {
      list.append(QString("\"%1\"").arg(QString::fromStdString(s)));
    }
    else
    {
      list.append(QString::fromStdString(s));
    }
  }
  list.sort(Qt::CaseInsensitive);
  model_->setStringList(list);
}
