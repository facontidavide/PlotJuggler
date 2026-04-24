#pragma once

#include <QCompleter>

#include "complete.h"

class QStringListModel;

class MetadataCompleter : public QCompleter
{
  Q_OBJECT

public:
  explicit MetadataCompleter(QObject* parent = nullptr);

  void update(const Completions& completions);

private:
  QStringListModel* model_ = nullptr;
};
