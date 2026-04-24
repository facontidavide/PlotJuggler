/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

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
