#include "blf_reader.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <limits>

#include <QDebug>

#if defined(PJ_HAS_VECTOR_BLF) && PJ_HAS_VECTOR_BLF
#if __has_include(<Vector/BLF/CanFdMessage64.h>) && __has_include(<Vector/BLF/CanMessage.h>) && \
    __has_include(<Vector/BLF/CanMessage2.h>) && __has_include(<Vector/BLF/File.h>)
#include <Vector/BLF/CanFdMessage64.h>
#include <Vector/BLF/CanMessage.h>
#include <Vector/BLF/CanMessage2.h>
#include <Vector/BLF/File.h>
#define PJ_CAN_USE_VECTOR_BLF 1
#if __has_include(<Vector/BLF/CanFdMessage.h>)
#include <Vector/BLF/CanFdMessage.h>
#define PJ_CAN_HAS_CANFD_MESSAGE 1
#endif
#if __has_include(<Vector/BLF/ObjectHeader.h>)
#include <Vector/BLF/ObjectHeader.h>
#define PJ_CAN_HAS_OBJECT_HEADER 1
#endif
#if __has_include(<Vector/BLF/ObjectHeader2.h>)
#include <Vector/BLF/ObjectHeader2.h>
#define PJ_CAN_HAS_OBJECT_HEADER2 1
#endif
#endif
#endif

#if !defined(PJ_CAN_USE_VECTOR_BLF)
#define PJ_CAN_USE_VECTOR_BLF 0
#endif
#if !defined(PJ_CAN_HAS_CANFD_MESSAGE)
#define PJ_CAN_HAS_CANFD_MESSAGE 0
#endif
#if !defined(PJ_CAN_HAS_OBJECT_HEADER)
#define PJ_CAN_HAS_OBJECT_HEADER 0
#endif
#if !defined(PJ_CAN_HAS_OBJECT_HEADER2)
#define PJ_CAN_HAS_OBJECT_HEADER2 0
#endif

namespace PJ::BLF
{
namespace
{

constexpr uint32_t kExtendedCanFlag = 0x80000000U;
constexpr uint32_t kStandardCanIdMask = 0x7FFU;
constexpr uint32_t kExtendedCanIdMask = 0x1FFFFFFFU;
constexpr uint8_t kMaxClassicCanPayload = 8U;
constexpr uint8_t kDlcToSize[16] = {0U,  1U,  2U,  3U,  4U,  5U,  6U,  7U,
                                     8U,  12U, 16U, 20U, 24U, 32U, 48U, 64U};
constexpr uint32_t kCanFd64EdlFlag = 0x1000U;
constexpr uint32_t kCanFd64BrsFlag = 0x2000U;
constexpr uint32_t kCanFd64EsiFlag = 0x4000U;

inline uint8_t ClampClassicCanSize(uint8_t dlc)
{
  return std::min(dlc, kMaxClassicCanPayload);
}

inline uint8_t DlcToFdSize(uint8_t dlc)
{
  return kDlcToSize[dlc & 0x0FU];
}

inline bool IsExtendedId(uint32_t id)
{
  return (id & kExtendedCanFlag) != 0U;
}

inline uint32_t StripCanId(uint32_t id, bool extended)
{
  return id & (extended ? kExtendedCanIdMask : kStandardCanIdMask);
}

#if PJ_CAN_USE_VECTOR_BLF
double HeaderTimestampToSeconds(const Vector::BLF::ObjectHeaderBase& base)
{
#if PJ_CAN_HAS_OBJECT_HEADER2
  if (const auto* header2 = dynamic_cast<const Vector::BLF::ObjectHeader2*>(&base))
  {
    const uint32_t flags = static_cast<uint32_t>(header2->objectFlags);
    const uint32_t one_nanosecond =
        static_cast<uint32_t>(Vector::BLF::ObjectHeader2::ObjectFlags::TimeOneNans);
    const double scale = (flags & one_nanosecond) ? 1e-9 : 1e-5;
    return static_cast<double>(header2->objectTimeStamp) * scale;
  }
#endif

#if PJ_CAN_HAS_OBJECT_HEADER
  if (const auto* header = dynamic_cast<const Vector::BLF::ObjectHeader*>(&base))
  {
    const uint32_t flags = static_cast<uint32_t>(header->objectFlags);
    const uint32_t one_nanosecond =
        static_cast<uint32_t>(Vector::BLF::ObjectHeader::ObjectFlags::TimeOneNans);
    const double scale = (flags & one_nanosecond) ? 1e-9 : 1e-5;
    return static_cast<double>(header->objectTimeStamp) * scale;
  }
#endif

  static bool warned_once = false;
  if (!warned_once)
  {
    warned_once = true;
    qWarning() << "DataLoadBLF: unsupported BLF object header timestamp format, fallback to 0.0";
  }
  return 0.0;
}

NormalizedCanFrame ToFrame(const Vector::BLF::CanMessage2& msg)
{
  NormalizedCanFrame frame;
  frame.timestamp = HeaderTimestampToSeconds(msg);
  frame.channel = msg.channel;
  frame.extended = IsExtendedId(msg.id);
  frame.id = StripCanId(msg.id, frame.extended);
  frame.dlc = msg.dlc;
  frame.size = ClampClassicCanSize(msg.dlc);
  frame.is_fd = false;
  frame.is_brs = false;
  frame.is_esi = false;
  std::copy_n(msg.data.begin(), frame.size, frame.data.begin());
  return frame;
}

NormalizedCanFrame ToFrame(const Vector::BLF::CanMessage& msg)
{
  NormalizedCanFrame frame;
  frame.timestamp = HeaderTimestampToSeconds(msg);
  frame.channel = msg.channel;
  frame.extended = IsExtendedId(msg.id);
  frame.id = StripCanId(msg.id, frame.extended);
  frame.dlc = msg.dlc;
  frame.size = ClampClassicCanSize(msg.dlc);
  frame.is_fd = false;
  frame.is_brs = false;
  frame.is_esi = false;
  std::copy_n(msg.data.begin(), frame.size, frame.data.begin());
  return frame;
}

NormalizedCanFrame ToFrame(const Vector::BLF::CanFdMessage64& msg)
{
  NormalizedCanFrame frame;
  frame.timestamp = HeaderTimestampToSeconds(msg);
  frame.channel = msg.channel;
  frame.extended = IsExtendedId(msg.id);
  frame.id = StripCanId(msg.id, frame.extended);
  frame.dlc = msg.dlc;
  frame.is_fd = (msg.flags & kCanFd64EdlFlag) != 0U;
  frame.is_brs = (msg.flags & kCanFd64BrsFlag) != 0U;
  frame.is_esi = (msg.flags & kCanFd64EsiFlag) != 0U;

  const uint8_t payload_size = msg.validDataBytes ? msg.validDataBytes : DlcToFdSize(msg.dlc);
  frame.size = std::min<uint8_t>(payload_size, static_cast<uint8_t>(frame.data.size()));
  std::copy_n(msg.data.begin(), frame.size, frame.data.begin());
  return frame;
}

#if PJ_CAN_HAS_CANFD_MESSAGE
NormalizedCanFrame ToFrame(const Vector::BLF::CanFdMessage& msg)
{
  NormalizedCanFrame frame;
  frame.timestamp = HeaderTimestampToSeconds(msg);
  frame.channel = msg.channel;
  frame.extended = IsExtendedId(msg.id);
  frame.id = StripCanId(msg.id, frame.extended);
  frame.dlc = msg.dlc;
  frame.is_fd = (msg.canFdFlags & Vector::BLF::CanFdMessage::CanFdFlags::EDL) != 0U;
  frame.is_brs = (msg.canFdFlags & Vector::BLF::CanFdMessage::CanFdFlags::BRS) != 0U;
  frame.is_esi = (msg.canFdFlags & Vector::BLF::CanFdMessage::CanFdFlags::ESI) != 0U;

  const uint8_t payload_size = msg.validDataBytes ? msg.validDataBytes : DlcToFdSize(msg.dlc);
  frame.size = std::min<uint8_t>(payload_size, static_cast<uint8_t>(frame.data.size()));
  std::copy_n(msg.data.begin(), frame.size, frame.data.begin());
  return frame;
}
#endif
#endif

}  // namespace

bool IsPlausibleUnixEpochSeconds(double seconds)
{
  constexpr double kMinUnixSeconds = 946684800.0;
  constexpr double kMaxUnixSeconds = 4102444800.0;
  return std::isfinite(seconds) && seconds >= kMinUnixSeconds && seconds <= kMaxUnixSeconds;
}

qint64 UnixEpochSecondsToMsec(double seconds, bool* ok)
{
  const bool valid = IsPlausibleUnixEpochSeconds(seconds);
  if (ok)
  {
    *ok = valid;
  }
  if (!valid)
  {
    return 0;
  }
  return static_cast<qint64>(seconds * 1000.0);
}

int ComputeBlfReadPercentage(uint32_t current_object, uint32_t total_objects)
{
  if (total_objects == 0U)
  {
    return 0;
  }
  const uint64_t scaled =
      (static_cast<uint64_t>(std::min(current_object, total_objects)) * 100ULL) / total_objects;
  return static_cast<int>(std::min<uint64_t>(scaled, 100ULL));
}

bool BlfReader::ReadFrames(const QString& path,
                           const std::function<void(const NormalizedCanFrame&)>& on_frame,
                           QString& error,
                           BlfLoadMetadata* metadata,
                           const std::function<bool(const BlfReadProgress&)>& on_progress) const
{
  error.clear();
  if (metadata)
  {
    metadata->has_valid_absolute_time = false;
    metadata->first_absolute_time_msec = 0;
  }

  if (path.trimmed().isEmpty())
  {
    error = "BLF path is empty";
    return false;
  }

#if PJ_CAN_USE_VECTOR_BLF
  try
  {
    Vector::BLF::File file;
    file.open(path.toStdString());
    if (!file.is_open())
    {
      error = QString("Failed to open BLF file: %1").arg(path);
      return false;
    }

    const uint32_t total_objects = file.fileStatistics.objectCount;
    auto report_progress = [&]() -> bool {
      if (!on_progress)
      {
        return true;
      }
      BlfReadProgress progress;
      progress.current_object = file.currentObjectCount.load();
      progress.total_objects = total_objects;
      progress.percentage = ComputeBlfReadPercentage(progress.current_object, progress.total_objects);
      return on_progress(progress);
    };

    if (!report_progress())
    {
      error = "BLF loading cancelled";
      file.close();
      return false;
    }

    while (file.good())
    {
      std::unique_ptr<Vector::BLF::ObjectHeaderBase> object(file.read());
      if (!object)
      {
        break;
      }

      if (const auto* msg = dynamic_cast<const Vector::BLF::CanMessage2*>(object.get()))
      {
        const auto frame = ToFrame(*msg);
        if (metadata && !metadata->has_valid_absolute_time)
        {
          bool ok = false;
          const qint64 absolute_time_msec = UnixEpochSecondsToMsec(frame.timestamp, &ok);
          if (ok)
          {
            metadata->first_absolute_time_msec = absolute_time_msec;
            metadata->has_valid_absolute_time = true;
          }
        }
        if (on_frame)
        {
          on_frame(frame);
        }
        if (!report_progress())
        {
          error = "BLF loading cancelled";
          file.close();
          return false;
        }
        continue;
      }

      if (const auto* msg = dynamic_cast<const Vector::BLF::CanFdMessage64*>(object.get()))
      {
        const auto frame = ToFrame(*msg);
        if (metadata && !metadata->has_valid_absolute_time)
        {
          bool ok = false;
          const qint64 absolute_time_msec = UnixEpochSecondsToMsec(frame.timestamp, &ok);
          if (ok)
          {
            metadata->first_absolute_time_msec = absolute_time_msec;
            metadata->has_valid_absolute_time = true;
          }
        }
        if (on_frame)
        {
          on_frame(frame);
        }
        if (!report_progress())
        {
          error = "BLF loading cancelled";
          file.close();
          return false;
        }
        continue;
      }

#if PJ_CAN_HAS_CANFD_MESSAGE
      if (const auto* msg = dynamic_cast<const Vector::BLF::CanFdMessage*>(object.get()))
      {
        const auto frame = ToFrame(*msg);
        if (metadata && !metadata->has_valid_absolute_time)
        {
          bool ok = false;
          const qint64 absolute_time_msec = UnixEpochSecondsToMsec(frame.timestamp, &ok);
          if (ok)
          {
            metadata->first_absolute_time_msec = absolute_time_msec;
            metadata->has_valid_absolute_time = true;
          }
        }
        if (on_frame)
        {
          on_frame(frame);
        }
        if (!report_progress())
        {
          error = "BLF loading cancelled";
          file.close();
          return false;
        }
        continue;
      }
#endif

      if (const auto* msg = dynamic_cast<const Vector::BLF::CanMessage*>(object.get()))
      {
        const auto frame = ToFrame(*msg);
        if (metadata && !metadata->has_valid_absolute_time)
        {
          bool ok = false;
          const qint64 absolute_time_msec = UnixEpochSecondsToMsec(frame.timestamp, &ok);
          if (ok)
          {
            metadata->first_absolute_time_msec = absolute_time_msec;
            metadata->has_valid_absolute_time = true;
          }
        }
        if (on_frame)
        {
          on_frame(frame);
        }
        if (!report_progress())
        {
          error = "BLF loading cancelled";
          file.close();
          return false;
        }
      }
    }

    if (on_progress)
    {
      on_progress(BlfReadProgress{total_objects, total_objects,
                                  ComputeBlfReadPercentage(total_objects, total_objects)});
    }

    file.close();
    return true;
  }
  catch (const std::exception& ex)
  {
    error = QString("BLF read error: %1").arg(ex.what());
    return false;
  }
#else
  (void)on_frame;
  (void)metadata;
  error = "vector_blf support is not available in this build";
  return false;
#endif
}

}  // namespace PJ::BLF
