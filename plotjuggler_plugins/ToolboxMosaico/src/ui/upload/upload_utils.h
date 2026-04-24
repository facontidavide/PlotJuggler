#pragma once

#include <QFileInfo>
#include <QString>

#include <set>

// Returns `stem` if not in existing, otherwise `stem_1`, `stem_2`, ...
[[nodiscard]] inline QString resolveSequenceName(
    const QString& stem, const std::set<QString>& existing)
{
  if (existing.find(stem) == existing.end()) return stem;
  for (int i = 1;; ++i)
  {
    QString candidate = stem + "_" + QString::number(i);
    if (existing.find(candidate) == existing.end()) return candidate;
  }
}

// True if the file has an extension that the Mosaico injector accepts.
[[nodiscard]] inline bool acceptsMcapFile(const QString& path)
{
  if (path.isEmpty()) return false;
  QString ext = QFileInfo(path).suffix().toLower();
  return ext == "mcap" || ext == "bag" || ext == "db3";
}
