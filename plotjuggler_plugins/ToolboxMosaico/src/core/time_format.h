#pragma once

#include <cstdint>
#include <string>

// Format a nanosecond timestamp as "hh:mm:ss" (short) or "dd/MM hh:mm:ss" (long).
std::string formatTimestamp(int64_t ts_ns, bool long_format);

// Format a nanosecond duration as human-readable: "42s", "12m 30s", "3h 45m", "2d 5h 30m".
std::string formatDuration(int64_t duration_ns);

// Returns true if the span exceeds 24 hours (use long timestamp format).
bool needsLongFormat(int64_t span_ns);
