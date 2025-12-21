
#include "line_parser.h"

using namespace PJ;

class MsgParserImpl : public MessageParser
{
public:
  MsgParserImpl(const std::string& topic_name, const std::string& type_name, const std::string&,
                PJ::PlotDataMapRef& data)
    : MessageParser(topic_name, data), topic_name_(topic_name)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override
  {
    auto ToChar = [](const auto* ptr) { return reinterpret_cast<const char*>(ptr); };

    const auto str = QString::fromLocal8Bit(ToChar(msg.data()), msg.size());

    std::string key;
    std::string prefix;
    // Obtain the key name from measurement name and tags
    for (auto line : str.splitRef('\n', PJ::SkipEmptyParts))
    {
      auto parts = line.split(' ', PJ::SkipEmptyParts);
      if (parts.size() != 2 && parts.size() != 3)
      {
        continue;
      }
      const auto tags = parts[0].split(',', PJ::SkipEmptyParts);
      const auto fields = parts[1].split(',', PJ::SkipEmptyParts);
      if (tags.size() < 1 || fields.size() < 1)
      {
        continue;
      }
      uint64_t timestamp = 0;
      if (parts.size() == 3)
      {
        timestamp = parts[2].toULongLong();
      }
      else
      {
        using namespace std::chrono;
        auto now = steady_clock::now();
        timestamp = duration_cast<nanoseconds>(now.time_since_epoch()).count();
      }
      const double ts_sec = double(timestamp) * 1e-9;

      prefix = topic_name_;
      for (auto tag : tags)
      {
        prefix += '/';
        auto tag_str = tag.toLocal8Bit();
        prefix.append(tag_str.data(), tag_str.size());
      }
      for (auto field : fields)
      {
        const auto field_parts = field.split('=');
        const auto name = field_parts[0].toLocal8Bit();
        auto value = field_parts[1].toLocal8Bit();

        key = prefix;
        key += '/';
        key.append(name.data(), name.size());

        if (value.startsWith('"') && value.endsWith('"'))
        {
          auto& data = _plot_data.getOrCreateStringSeries(key);
          data.pushBack(
              PJ::StringSeries::Point(ts_sec, StringRef(value.data() + 1, value.size() - 2)));
        }
        else if (value == "t" || value == "T" || value == "true" || value == "True" ||
                 value == "TRUE")
        {
          auto& data = _plot_data.getOrCreateNumeric(key);
          data.pushBack({ ts_sec, 1.0 });
        }
        else if (value == "f" || value == "F" || value == "false" || value == "False" ||
                 value == "FALSE")
        {
          auto& data = _plot_data.getOrCreateNumeric(key);
          data.pushBack({ ts_sec, 0.0 });
        }
        else
        {
          bool ok = false;
          // remove last character if there is an integer suffix
          if (value.endsWith('i') || value.endsWith('u'))
          {
            value.chop(1);
          }
          double num = value.toDouble(&ok);
          if (ok)
          {
            auto& data = _plot_data.getOrCreateNumeric(key);
            data.pushBack({ ts_sec, num });
          }
        }
      }
    }
    return true;
  }

private:
  std::string topic_name_;
};

MessageParserPtr ParserLine::createParser(const std::string& topic_name,
                                          const std::string& type_name, const std::string& schema,
                                          PJ::PlotDataMapRef& data)
{
  return std::make_shared<MsgParserImpl>(topic_name, type_name, schema, data);
}
