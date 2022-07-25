/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "nlohmann_parsers.h"

#include "PlotJuggler/fmt/format.h"

bool NlohmannParser::parseMessageImpl(double& timestamp)
{
  if (_use_message_stamp && _stamp_fieldname.empty() == false)
  {
    auto ts = _json.find(_stamp_fieldname);
    if (ts != _json.end() && ts.value().is_number())
    {
      timestamp = ts.value().get<double>();
    }
    else
    {
      _use_message_stamp = false;
    }
  }

  std::function<void(const std::string&, const nlohmann::json&)> flatten;

  flatten = [&](const std::string& prefix, const nlohmann::json& value) {
    if (value.empty())
    {
      return;
    }

    switch (value.type())
    {
      case nlohmann::detail::value_t::array: {
        // iterate array and use index as reference string
        for (std::size_t i = 0; i < value.size(); ++i)
        {
          flatten(fmt::format("{}[{}]", prefix, i), value[i]);
        }
        break;
      }

      case nlohmann::detail::value_t::object: {
        // iterate object and use keys as reference string
        for (const auto& element : value.items())
        {
          flatten(fmt::format("{}/{}", prefix, element.key()), element.value());
        }
        break;
      }

      default: {
        double numeric_value = 0;
        if (value.is_boolean())
        {
          numeric_value = value.get<bool>();
        }
        else if (value.is_number())
        {
          numeric_value = value.get<double>();
        }
        else
        {
          return;
        }

        auto plot_data = &(getSeries(prefix));
        plot_data->pushBack({ timestamp, numeric_value });

        break;
      }
    }  // end switch
  };

  flatten(_topic_name, _json);
  return true;
}

bool MessagePack_Parser::parseMessage(const MessageRef msg, double& timestamp)
{
  _json = nlohmann::json::from_msgpack(msg.data(), msg.data() + msg.size());
  return parseMessageImpl(timestamp);
}

bool JSON_Parser::parseMessage(const MessageRef msg, double& timestamp)
{
  _json = nlohmann::json::parse(msg.data(), msg.data() + msg.size());
  return parseMessageImpl(timestamp);
}

bool CBOR_Parser::parseMessage(const MessageRef msg, double& timestamp)
{
  _json = nlohmann::json::from_cbor(msg.data(), msg.data() + msg.size());
  return parseMessageImpl(timestamp);
}

bool BSON_Parser::parseMessage(const MessageRef msg, double& timestamp)
{
  _json = nlohmann::json::from_bson(msg.data(), msg.data() + msg.size());
  return parseMessageImpl(timestamp);
}
