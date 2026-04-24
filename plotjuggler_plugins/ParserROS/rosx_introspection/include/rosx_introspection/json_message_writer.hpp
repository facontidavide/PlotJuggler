#pragma once

#include <memory>

#include "rosx_introspection/message_writer.hpp"

#ifdef ROSX_HAS_JSON

namespace RosMsgParser {

class JsonMessageWriter : public MessageWriter {
 public:
  JsonMessageWriter(void* doc, bool ignore_constants);
  ~JsonMessageWriter() override;

  JsonMessageWriter(const JsonMessageWriter&) = delete;
  JsonMessageWriter& operator=(const JsonMessageWriter&) = delete;

  void writeValue(const FieldLeaf& leaf, const Variant& value) override;
  void writeString(const FieldLeaf& leaf, const std::string& str) override;
  void writeEnum(const FieldLeaf& leaf, int32_t value, const std::string& name) override;
  void beginStruct(const ROSField& field) override;
  void endStruct() override;

 private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};

}  // namespace RosMsgParser

#endif  // ROSX_HAS_JSON
