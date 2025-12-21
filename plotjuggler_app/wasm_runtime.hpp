#pragma once

#include <wasmtime.hh>
#include <wasmtime/wasi.hh>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class WasmRuntime
{
public:
  WasmRuntime(const std::string& module_path);
  ~WasmRuntime() = default;

  // Non-copyable
  WasmRuntime(const WasmRuntime&) = delete;
  WasmRuntime& operator=(const WasmRuntime&) = delete;

  wasmtime::Func getFunc(const std::string& name);

  std::string wasmValueToString(const wasmtime::Val& val);

  // String parameter helpers

  int32_t allocateBuffer(const void* data, size_t size);

  // similar to allocateBuffer, but adds the null terminator
  int32_t allocateString(const std::string& str);

  void freeWasmMemory(int32_t ptr);

  wasmtime::Store& store()
  {
    return store_;
  }

  uint8_t* memoryPointer(int32_t ptr = 0)
  {
    return memory_->data(store_).data() + ptr;
  }

private:
  wasmtime::Engine engine_;
  wasmtime::Store store_;
  std::optional<wasmtime::Module> module_;
  std::optional<wasmtime::Instance> instance_;
  wasmtime::Linker linker_;
  std::optional<wasmtime::Memory> memory_;

  void defineStubImports();
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
  operator wasmtime::Val() const
  {
    return wasmtime::Val(ptr_);
  }

private:
  WasmRuntime* runtime_;
  int32_t ptr_;
};

std::string readPluginManifest(WasmRuntime& runtime);
