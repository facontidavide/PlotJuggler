#pragma once

#include "PlotJuggler/messageparser_base.h"
#include "wasm_runtime.hpp"

namespace PJ
{

class ParserFactoryWASM : public ParserFactoryPlugin
{
public:
  ParserFactoryWASM(std::unique_ptr<WasmRuntime> runtime, QString plugin_name, QString encoding);
  ~ParserFactoryWASM() = default;

  const char* name() const override
  {
    return _plugin_name.toStdString().c_str();
  }
  const char* encoding() const override
  {
    return _encoding.toStdString().c_str();
  }

  // Message parsing
  MessageParserPtr createParser(const std::string& topic_name, const std::string& type_name,
                                const std::string& schema, PlotDataMapRef& data) override;

private:
  std::shared_ptr<WasmRuntime> _runtime;
  QString _plugin_name;
  QString _encoding;
};

}  // namespace PJ
