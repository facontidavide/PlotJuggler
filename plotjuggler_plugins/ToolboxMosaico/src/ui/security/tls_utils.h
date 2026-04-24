#pragma once

#include <QFileInfo>
#include <QRegularExpression>
#include <QString>

// Returns true if the key matches: msco_[32 lowercase alnum]_[8 hex chars]
[[nodiscard]] inline bool isValidApiKey(const QString& key)
{
  static const QRegularExpression re(
      R"(^msco_[a-z0-9]{32}_[0-9a-f]{8}$)");
  return re.match(key).hasMatch();
}

// Returns true if the file exists and is readable.
[[nodiscard]] inline bool isCertReadable(const QString& path)
{
  if (path.isEmpty()) return false;
  QFileInfo fi(path);
  return fi.exists() && fi.isReadable();
}

