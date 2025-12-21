# WASM plugins

All wasm plugin should expose a function of type


```cpp
const char* pj_plugin_manifest();
```

This function will return the following YAML manifest. Currently we support only Messageparser

```yaml
plugin_type: MessageParser # only one supported for the time being
name: ParserFactoryROS2
version: 0.1.0
description: Plugin to parse DDS messages (e.g., from ROS2)
author: Davide Faconti
license: AGPL
url: https://github.com/facontidavide/rosx_introspection

encoding: "ros2msg" # mandatory for MessageParser type

parameters_model: # Optional

    - ID: label_1 # identifier of the label
      type: Label
      label: "This is just text"

    - ID: host_name # contains text
      label: "Host IP:"
      type: LineEdit
      default_value: "127.0.0.1"                  # Optional
      description: "ID address must be numerical" # Optional
      regexp_validation: "[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}" # Optional

    - ID: port_number
      label: "Port number:"
      type: SpinBoxInt
      description: "Port number" # Optional
      default_value: 1888        # Optional
      range: [0, 9999]           # Optional: vector with min/max pair

    - ID: real_value
      label: "Multiplier:"
      type: SpinBoxDouble
      description: "Multiplier value" # Optional
      default_value: 3.14             # Optional
      range: [0, 10]                  # Optional: vector with min/max pair

    - ID: debug_mode
      type: CheckBox
      default_value: true  # Optional. False if missing
      description: "Enable debug mode" # Optional

    - ID: more_options
      type: ComboBox
      values: "YES;NO;ASK"        # Mandatory: must be semicolon separated
      default_value: "ASK"        # Optional
      description: "Autoconfirm?" # Optional
```


## MessageParser API

```cpp

void* pj_parser_create(const char* topic_name, const char* type_name, const char* schema);

void pj_parser_set_array_policy(void* self, bool clamp, uint32_t max_size);

uint32_t pj_parser_parse(void* self, const uint8_t* raw_data, uint32_t raw_data_len, uint8_t* output_buffer);

```

### Key/Value table serialization of "output_buffer"

Simple serialization rules (little endian will be used used):

1. A string is always represented as a **uint16_t** with the number of characters (N), followed by the N character themselves.

2. A number is 1 bytes, representing **BuiltinType** followed by the number itself, where the amount of bytes is consistent with the type, for instance 2 for `int16_t`, 4 for `float` and 8 for `double`.

3. A blob is represented as 2 consecutive `uint32_t` numbers, being the first the offset in memory pointing to the beginning of the blob (with respect to the beginning of the raw message), followed by the size of the blob.


The "output_buffer" will contain a serialized list of key values, encoded as follows

- first 4 bytes of the **output_buffer** contains the number of key/value pairs.

- a "key" is always a string, while the "value" could be either a number, string or blob.


``` cpp
enum class BuiltinType : uint8_t
{
  UINT8 = 0,
  UINT16 = 1,
  UINT32 = 2,
  UINT64 = 3,
  INT8 = 8,
  INT16 = 9,
  INT32 = 10,
  INT64 = 11,
  FLOAT32 = 16,
  FLOAT64 = 17,
  STRING = 18,
  BLOB = 19,
  OTHER = 255
};
```
