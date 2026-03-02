#pragma once

#include <wasmer.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class WasmRuntime
{
public:
  WasmRuntime(const std::string& module_path);
  ~WasmRuntime();

  // Non-copyable
  WasmRuntime(const WasmRuntime&) = delete;
  WasmRuntime& operator=(const WasmRuntime&) = delete;

  // Returns borrowed handle; caller must NOT delete it
  wasm_func_t* getFunc(const std::string& name);

  // Call a wasm function with i32 args; returns i32 results
  std::vector<int32_t> callFunc(wasm_func_t* func, const std::vector<int32_t>& args);

  std::string wasmValueToString(int32_t str_ptr);

  int32_t allocateBuffer(const void* data, size_t size);
  int32_t allocateString(const std::string& str);
  void freeWasmMemory(int32_t ptr);

  uint8_t* memoryPointer(int32_t ptr = 0)
  {
    return reinterpret_cast<uint8_t*>(wasm_memory_data(memory_)) + ptr;
  }

private:
  wasm_engine_t* engine_ = nullptr;
  wasm_store_t* store_ = nullptr;
  wasm_module_t* module_ = nullptr;
  wasm_instance_t* instance_ = nullptr;
  wasm_memory_t* memory_ = nullptr;  // borrowed from exports_
  wasi_env_t* wasi_env_ = nullptr;

  // Host (stub) functions kept alive for instance lifetime
  std::vector<wasm_func_t*> host_funcs_;
  // Export externs from wasm_instance_exports (owned)
  wasm_extern_vec_t exports_ = WASM_EMPTY_VEC;
  // Name → borrowed func pointer (valid while exports_ is alive)
  std::unordered_map<std::string, wasm_func_t*> export_funcs_;

  void buildAndInstantiate();
};

class WasmString
{
public:
  WasmString(WasmRuntime& runtime, const std::string& str);
  ~WasmString();

  // Non-copyable
  WasmString(const WasmString&) = delete;
  WasmString& operator=(const WasmString&) = delete;

  // Movable
  WasmString(WasmString&& other) noexcept;
  WasmString& operator=(WasmString&& other) noexcept;

  int32_t ptr() const
  {
    return ptr_;
  }

private:
  WasmRuntime* runtime_;
  int32_t ptr_;
};

std::string readPluginManifest(WasmRuntime& runtime);
