#include "server_history.h"

namespace
{
constexpr const char* kGrpcScheme = "grpc://";
constexpr const char* kGrpcTlsScheme = "grpc+tls://";
}  // namespace

QString normalizeServerKey(const QString& uri)
{
  QString s = uri.trimmed();
  if (s.isEmpty()) return {};

  if (s.startsWith(kGrpcTlsScheme))
    s.remove(0, static_cast<int>(qstrlen(kGrpcTlsScheme)));
  else if (s.startsWith(kGrpcScheme))
    s.remove(0, static_cast<int>(qstrlen(kGrpcScheme)));

  if (s.endsWith('/')) s.chop(1);
  if (s.isEmpty()) return {};

  const int colon = s.indexOf(':');
  if (colon < 0)
  {
    return s.toLower();
  }

  QString host = s.left(colon).toLower();
  QString rest = s.mid(colon);
  return host + rest;
}

QStringList promoteToHead(const QStringList& history, const QString& key,
                          int cap)
{
  if (key.isEmpty()) return history;
  if (cap <= 0) return {};

  QStringList out;
  out.reserve(cap);
  out.append(key);
  for (const auto& entry : history)
  {
    if (entry == key) continue;
    if (out.size() >= cap) break;
    out.append(entry);
  }
  return out;
}
