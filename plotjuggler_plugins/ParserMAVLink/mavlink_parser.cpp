#include <QSettings>
#include <QMessageBox>
#include "mavlink_parser.h"
#include "PlotJuggler/fmt/format.h"
#include "PlotJuggler/svg_util.h"

bool MAVLinkParser::parseMessage(const MessageRef serialized_msg,
                                  double &timestamp)
{
  for (int position = 0; position < serialized_msg.size(); position++) {
    if (mavlink_parse_char(MAVLINK_COMM_0, static_cast<uint8_t>(serialized_msg.data()[position]), &_message, &_status)) {
      const mavlink_message_info_t* msgInfo = mavlink_get_message_info(&_message);
      if (msgInfo) {
        uint8_t* m = reinterpret_cast<uint8_t*>(&_message.payload64[0]);
        for (unsigned int i = 0; i < msgInfo->num_fields; ++i) {
          const unsigned int offset = msgInfo->fields[i].wire_offset;
          const unsigned int array_length = msgInfo->fields[i].array_length;
          static const unsigned int array_buffer_length = (MAVLINK_MAX_PAYLOAD_LEN + MAVLINK_NUM_CHECKSUM_BYTES + 7);
          double value = 0;
          std::string suffix;
          std::string key = fmt::format("{}/{}", msgInfo->name, msgInfo->fields[i].name );
          switch (msgInfo->fields[i].type) {
            case MAVLINK_TYPE_CHAR:
            {
              std::string strValue;
              if (array_length > 0) {
                char* str = reinterpret_cast<char*>(m + offset);
                // Enforce null termination
                str[array_length - 1] = '\0';
                strValue = str;
              } else {
                // Single char
                char b = *(reinterpret_cast<char*>(m + offset));
                strValue = b;
              }
              auto& series = this->getStringSeries(key);
              series.pushBack({timestamp, strValue});
            }
              break;
            case MAVLINK_TYPE_UINT8_T:
              if (array_length > 0) {
                uint8_t* nums = m + offset;
                for (unsigned int index = 0; index < array_length; ++index) {
                  suffix = fmt::format("[{}]", index);
                  auto& series = this->getSeries(key + suffix);
                  value = static_cast<double>(nums[index]);
                  series.pushBack({timestamp, value});
                }
              } else {
                // Single value
                uint8_t u = *(m + offset);
                value = static_cast<double>(u);
                auto& series = this->getSeries(key);
                series.pushBack({timestamp, value});
              }
              break;
            case MAVLINK_TYPE_INT8_T:
              if (array_length > 0) {
                int8_t* nums = reinterpret_cast<int8_t*>(m + offset);
                for (unsigned int index = 0; index < array_length; ++index) {
                  suffix = fmt::format("[{}]", index);
                  auto& series = this->getSeries(key + suffix);
                  value = static_cast<double>(nums[index]);
                  series.pushBack({timestamp, value});
                }
              } else {
                // Single value
                int8_t n = *(reinterpret_cast<int8_t*>(m + offset));
                value = static_cast<double>(n);
                auto& series = this->getSeries(key);
                series.pushBack({timestamp, value});
              }
              break;
            case MAVLINK_TYPE_UINT16_T:
              if (array_length > 0) {
                uint16_t nums[array_buffer_length / sizeof(uint16_t)];
                memcpy(nums, m + offset,  sizeof(uint16_t) * array_length);
                for (unsigned int index = 0; index < array_length; ++index) {
                  suffix = fmt::format("[{}]", index);
                  auto& series = this->getSeries(key + suffix);
                  value = static_cast<double>(nums[index]);
                  series.pushBack({timestamp, value});
                }
              } else {
                // Single value
                uint16_t n;
                memcpy(&n, m + offset, sizeof(uint16_t));
                value = static_cast<double>(n);
                auto& series = this->getSeries(key);
                series.pushBack({timestamp, value});
              }
              break;
            case MAVLINK_TYPE_INT16_T:
              if (array_length > 0) {
                int16_t nums[array_buffer_length / sizeof(int16_t)];
                memcpy(nums, m + offset,  sizeof(int16_t) * array_length);
                for (unsigned int index = 0; index < array_length; ++index) {
                  suffix = fmt::format("[{}]", index);
                  auto& series = this->getSeries(key + suffix);
                  value = static_cast<double>(nums[index]);
                  series.pushBack({timestamp, value});
                }
              } else {
                // Single value
                int16_t n;
                memcpy(&n, m + offset, sizeof(int16_t));
                value = static_cast<double>(n);
                auto& series = this->getSeries(key);
                series.pushBack({timestamp, value});
              }
              break;
            case MAVLINK_TYPE_UINT32_T:
              if (array_length > 0) {
                uint32_t nums[array_buffer_length / sizeof(uint32_t)];
                memcpy(nums, m + offset,  sizeof(uint32_t) * array_length);
                for (unsigned int index = 0; index < array_length; ++index) {
                  suffix = fmt::format("[{}]", index);
                  auto& series = this->getSeries(key + suffix);
                  value = static_cast<double>(nums[index]);
                  series.pushBack({timestamp, value});
                }
              } else {
                // Single value
                uint32_t n;
                memcpy(&n, m + offset, sizeof(uint32_t));
                value = static_cast<double>(n);
                auto& series = this->getSeries(key);
                series.pushBack({timestamp, value});
              }
              break;
            case MAVLINK_TYPE_INT32_T:
              if (array_length > 0) {
                int32_t nums[array_buffer_length / sizeof(int32_t)];
                memcpy(nums, m + offset,  sizeof(int32_t) * array_length);
                for (unsigned int index = 0; index < array_length; ++index) {
                  suffix = fmt::format("[{}]", index);
                  auto& series = this->getSeries(key + suffix);
                  value = static_cast<double>(nums[index]);
                  series.pushBack({timestamp, value});
                }
              } else {
                // Single value
                int32_t n;
                memcpy(&n, m + offset, sizeof(int32_t));
                value = static_cast<double>(n);
                auto& series = this->getSeries(key);
                series.pushBack({timestamp, value});
              }
              break;
            case MAVLINK_TYPE_FLOAT:
              if (array_length > 0) {
                float nums[array_buffer_length / sizeof(float)];
                memcpy(nums, m + offset,  sizeof(float) * array_length);
                for (unsigned int index = 0; index < array_length; ++index) {
                  suffix = fmt::format("[{}]", index);
                  auto& series = this->getSeries(key + suffix);
                  value = static_cast<double>(nums[index]);
                  series.pushBack({timestamp, value});
                }
              } else {
                // Single value
                float n;
                memcpy(&n, m + offset, sizeof(float));
                value = static_cast<double>(n);
                auto& series = this->getSeries(key);
                series.pushBack({timestamp, value});
              }
              break;
            case MAVLINK_TYPE_DOUBLE:
              if (array_length > 0) {
                double nums[array_buffer_length / sizeof(double)];
                memcpy(nums, m + offset,  sizeof(double) * array_length);
                for (unsigned int index = 0; index < array_length; ++index) {
                  suffix = fmt::format("[{}]", index);
                  auto& series = this->getSeries(key + suffix);
                  value = nums[index];
                  series.pushBack({timestamp, value});
                }
              } else {
                // Single value
                double n;
                memcpy(&n, m + offset, sizeof(double));
                value = n;
                auto& series = this->getSeries(key);
                series.pushBack({timestamp, value});
              }
              break;
            case MAVLINK_TYPE_UINT64_T:
              if (array_length > 0) {
                uint64_t nums[array_buffer_length / sizeof(uint64_t)];
                memcpy(nums, m + offset,  sizeof(uint64_t) * array_length);
                for (unsigned int index = 0; index < array_length; ++index) {
                  suffix = fmt::format("[{}]", index);
                  auto& series = this->getSeries(key + suffix);
                  value = static_cast<double>(nums[index]);
                  series.pushBack({timestamp, value});
                }
              } else {
                // Single value
                uint64_t n;
                memcpy(&n, m + offset, sizeof(uint64_t));
                value = static_cast<double>(n);
                auto& series = this->getSeries(key);
                series.pushBack({timestamp, value});
              }
              break;
            case MAVLINK_TYPE_INT64_T:
              if (array_length > 0) {
                int64_t nums[array_buffer_length / sizeof(int64_t)];
                memcpy(nums, m + offset,  sizeof(int64_t) * array_length);
                for (unsigned int index = 0; index < array_length; ++index) {
                  suffix = fmt::format("[{}]", index);
                  auto& series = this->getSeries(key + suffix);
                  value = static_cast<double>(nums[index]);
                  series.pushBack({timestamp, value});
                }
              } else {
                // Single value
                int64_t n;
                memcpy(&n, m + offset, sizeof(int64_t));
                value = static_cast<double>(n);
                auto& series = this->getSeries(key);
                series.pushBack({timestamp, value});
              }
              break;
            default:
              break;
          }
        }
      }
    }
  }

  return true;
}

