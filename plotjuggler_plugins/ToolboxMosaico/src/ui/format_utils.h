#pragma once

#include <QString>
#include <cstdint>

/// Format a byte count as a human-readable string (e.g. "1.2 GB", "340 KB").
[[nodiscard]] inline QString formatBytes(int64_t bytes)
{
  if (bytes <= 0)
  {
    return {};
  }
  constexpr int64_t kKB = 1024;
  constexpr int64_t kMB = 1024 * kKB;
  constexpr int64_t kGB = 1024 * kMB;
  if (bytes >= kGB)
  {
    return QString("%1 GB").arg(static_cast<double>(bytes) / static_cast<double>(kGB), 0, 'f', 1);
  }
  if (bytes >= kMB)
  {
    return QString("%1 MB").arg(static_cast<double>(bytes) / static_cast<double>(kMB), 0, 'f', 1);
  }
  if (bytes >= kKB)
  {
    return QString("%1 KB").arg(static_cast<double>(bytes) / static_cast<double>(kKB), 0, 'f', 1);
  }
  return QString("%1 B").arg(bytes);
}
