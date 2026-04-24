#pragma once

#include <QString>
#include <QStringList>

// Normalize a user-supplied URI to a canonical `host:port` key used for
// dedup and per-server credential storage. Rules:
//   - trim surrounding whitespace
//   - strip leading `grpc://` or `grpc+tls://`
//   - drop a single trailing `/`
//   - lowercase everything up to the first `:` (the host); leave the port as-is
// Returns an empty QString for inputs that produce no usable key (empty
// input, scheme with no host, etc.).
[[nodiscard]] QString normalizeServerKey(const QString& uri);

// Produce a new MRU history list with `key` moved (or inserted) at the
// front, followed by all other entries in their previous order (with any
// prior occurrence of `key` removed). The result is truncated to at most
// `cap` entries. If `key` is empty the input list is returned unchanged.
// If `cap` is 0 the result is empty.
[[nodiscard]] QStringList promoteToHead(const QStringList& history,
                                        const QString& key,
                                        int cap);
