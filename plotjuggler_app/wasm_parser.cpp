#include "wasm_parser.hpp"
#include "PlotJuggler/pj_serializer.hpp"
#include "PlotJuggler/contrib/unordered_dense.hpp"

namespace PJ
{

struct string_hash
{
  using is_transparent = void;  // enable heterogeneous overloads
  using is_avalanching = void;  // mark class as high quality avalanching hash

  [[nodiscard]] auto operator()(std::string_view str) const noexcept -> uint64_t
  {
    return ankerl::unordered_dense::hash<std::string_view>{}(str);
  }
};

class MessageparserWASM : public MessageParser
{
public:
  MessageparserWASM(std::shared_ptr<WasmRuntime> runtime, const std::string& topic_name,
                    const std::string& type_name, const std::string& schema, PlotDataMapRef& data)
    : MessageParser(topic_name, data)
    , _runtime(std::move(runtime))
    , _topic_name(topic_name)
    , _wasm_topic_name(*_runtime, "")  // intentional empty
    , _wasm_topic_type(*_runtime, type_name)
    , _wasm_schema(*_runtime, schema)
    , _decode_func(_runtime->getFunc("pj_parser_decode"))
  {
    auto create_func = _runtime->getFunc("pj_parser_create");
    std::vector<wasmtime::Val> params = { _wasm_topic_name, _wasm_topic_type, _wasm_schema };
    auto results = create_func.call(_runtime->store(), params).unwrap();
    _parser_instance = results[0].i32();

    _parser_input_buffer_size = 128 * 1024;  // 128 KB
    _parser_input_buffer_ptr = _runtime->allocateBuffer(nullptr, _parser_input_buffer_size);
    _parser_output_buffer_ptr = _runtime->allocateBuffer(nullptr, 1024 * 1024);  // 1 MB
  }

  ~MessageparserWASM()
  {
    if (_parser_instance != 0)
    {
      auto destroy_func = _runtime->getFunc("pj_parser_destroy");
      destroy_func.call(_runtime->store(), { _parser_instance }).unwrap();
    }
    if (_parser_input_buffer_ptr != 0)
    {
      _runtime->freeWasmMemory(_parser_input_buffer_ptr);
    }
    if (_parser_output_buffer_ptr != 0)
    {
      _runtime->freeWasmMemory(_parser_output_buffer_ptr);
    }
  }

  bool parseMessage(const MessageRef serialized_msg, double& timestamp) override
  {
    if (_parser_input_buffer_size < serialized_msg.size())
    {
      // reallocate a larger input buffer, if necessary
      _runtime->freeWasmMemory(_parser_input_buffer_ptr);
      _parser_input_buffer_size =
          std::max(_parser_input_buffer_size * 2u, uint32_t(serialized_msg.size()));
      _parser_input_buffer_ptr = _runtime->allocateBuffer(nullptr, _parser_input_buffer_size);
    }
    // Original function signature:
    // int32_t pj_parser_decode(void* self, const uint8_t* raw_data, uint32_t raw_data_len,
    //                          uint8_t* output_buffer);

    // copy the memory into the input buffer
    uint8_t* input_data = _runtime->memoryPointer(_parser_input_buffer_ptr);
    const uint8_t* output_data = _runtime->memoryPointer(_parser_output_buffer_ptr);

    memcpy(input_data, serialized_msg.data(), serialized_msg.size());

    _params = { _parser_instance, _parser_input_buffer_ptr, int32_t(serialized_msg.size()),
                _parser_output_buffer_ptr };

    auto results = _decode_func.call(_runtime->store(), _params).unwrap();
    int32_t output_size = results[0].i32();
    if (output_size <= 0)
    {
      return false;
    }

    uint32_t pairs_count = 0;
    output_data += unpack_number(output_data, pairs_count);

    std::string value_str;
    for (uint32_t i = 0; i < pairs_count; i++)
    {
      // Extract key
      std::string_view key_str;
      output_data += unpack_string(output_data, key_str);
      BuiltinType type = static_cast<BuiltinType>(*output_data);
      output_data += 1;  // skip type byte

      if (key_str == "__timestamp")
      {
        // special key for embedded timestamp
        output_data += unpack_number_into_double(output_data, type, timestamp);
        continue;
      }

      if (type == BuiltinType::BLOB)
      {
        // skip the blob (offset + size)
        output_data += 8;
        continue;
      }
      if (type == BuiltinType::STRING)
      {
        std::string_view value_str;
        output_data += unpack_string(output_data, value_str);
        auto string_series_it = _strings_map.find(key_str);
        if (string_series_it == _strings_map.end())
        {
          std::string full_key = _topic_name + std::string(key_str);
          string_series_it = _strings_map.emplace(key_str, &getStringSeries(full_key)).first;
        }
        string_series_it->second->pushBack({ timestamp, value_str });
      }
      else
      {
        // cast to double
        double value = 0.0;
        output_data += unpack_number_into_double(output_data, type, value);
        auto num_series_it = _numbers_map.find(key_str);
        if (num_series_it == _numbers_map.end())
        {
          std::string full_key = _topic_name + std::string(key_str);
          num_series_it = _numbers_map.emplace(key_str, &getSeries(full_key)).first;
        }
        num_series_it->second->pushBack({ timestamp, value });
      }
    }
    return false;
  }

  void setLargeArraysPolicy(bool clamp, unsigned max_size)
  {
    MessageParser::setLargeArraysPolicy(clamp, max_size);
    // Original function signature:
    // void pj_parser_set_array_policy(void* self, bool clamp, uint32_t
    // max_size);
    auto set_policy_func = _runtime->getFunc("pj_parser_set_array_policy");
    _params = { _parser_instance, clamp, int32_t(max_size) };
    set_policy_func.call(_runtime->store(), _params).unwrap();
  }

private:
  std::shared_ptr<WasmRuntime> _runtime;
  std::string _topic_name;
  WasmString _wasm_topic_name;
  WasmString _wasm_topic_type;
  WasmString _wasm_schema;
  int32_t _parser_instance = 0;
  std::vector<wasmtime::Val> _params;
  wasmtime::Func _decode_func;
  int32_t _parser_input_buffer_size = 128 * 1024;  // 128 KB
  int32_t _parser_input_buffer_ptr = 0;
  int32_t _parser_output_buffer_ptr = 0;

  ankerl::unordered_dense::map<std::string, PlotData*, string_hash, std::equal_to<>> _numbers_map;
  ankerl::unordered_dense::map<std::string, StringSeries*, string_hash, std::equal_to<>>
      _strings_map;
};

ParserFactoryWASM::ParserFactoryWASM(std::unique_ptr<WasmRuntime> runtime, QString plugin_name,
                                     QString encoding)
  : _runtime(std::move(runtime))
  , _plugin_name(std::move(plugin_name))
  , _encoding(std::move(encoding))
{
  qDebug() << "ParserFactoryWASM Plugin: " << _plugin_name << " encoding: " << _encoding;
}

MessageParserPtr ParserFactoryWASM::createParser(const std::string& topic_name,
                                                 const std::string& type_name,
                                                 const std::string& schema, PlotDataMapRef& data)
{
  return std::make_shared<MessageparserWASM>(_runtime, topic_name, type_name, schema, data);
}

}  // namespace PJ
