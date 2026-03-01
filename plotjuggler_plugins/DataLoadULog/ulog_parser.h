#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

class ULogParser
{
public:
  enum FormatType
  {
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    INT8,
    INT16,
    INT32,
    INT64,
    FLOAT,
    DOUBLE,
    BOOL,
    CHAR,
    OTHER
  };

  struct Parameter
  {
    std::string name;
    union
    {
      int32_t val_int;
      float val_real;
    } value;
    FormatType val_type;
  };

  struct MessageLog
  {
    char level;
    uint64_t timestamp;
    std::string msg;
  };

  struct Timeseries
  {
    std::vector<std::optional<uint64_t>> timestamps;
    std::vector<std::pair<std::string, std::vector<double>>> data;
  };

public:
  ULogParser(const uint8_t* data, size_t length);

  const std::map<std::string, Timeseries>& getTimeseriesMap() const;

  const std::vector<Parameter>& getParameters() const;

  const std::map<std::string, std::string>& getInfo() const;

  const std::vector<MessageLog>& getLogs() const;

private:
  std::vector<Parameter> _parameters;
  std::map<std::string, std::string> _info;
  std::map<std::string, Timeseries> _timeseries;
  std::vector<MessageLog> _message_logs;
};
