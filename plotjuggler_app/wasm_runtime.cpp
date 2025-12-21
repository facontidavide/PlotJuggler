#include "wasm_runtime.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

std::vector<uint8_t> readBinaryFile(const std::string& path)
{
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
  {
    throw std::runtime_error("Failed to open WASM file: " + path);
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<uint8_t> buffer(size);
  if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
  {
    throw std::runtime_error("Failed to read WASM file");
  }
  return buffer;
}

// Stub function callbacks for C++ runtime using C++ API

void WasmRuntime::defineStubImports()
{
  // __cxa_allocate_exception(size) -> pointer
  auto allocate_exception_func = wasmtime::Func::wrap(store_, [](int32_t size) -> int32_t {
    return 1;  // Return dummy non-zero pointer
  });
  linker_.define(store_.context(), "env", "__cxa_allocate_exception", allocate_exception_func)
      .unwrap();

  // __cxa_throw(exception, type, destructor) -> no return (traps)
  auto throw_func =
      wasmtime::Func::wrap(store_, [](int32_t exception, int32_t type, int32_t destructor) {
        std::cerr << "Warning: WASM called __cxa_throw (exception=" << exception << ")"
                  << std::endl;
        // Don't throw immediately - let's see what happens
      });
  linker_.define(store_.context(), "env", "__cxa_throw", throw_func).unwrap();

  // abort() -> no return (traps)
  auto abort_func = wasmtime::Func::wrap(store_, []() {
    std::cerr << "Warning: WASM called abort()" << std::endl;
    // Don't throw immediately - let's see what happens
  });
  linker_.define(store_.context(), "env", "abort", abort_func).unwrap();
}

WasmRuntime::WasmRuntime(const std::string& module_path) : store_(engine_), linker_(engine_)
{
  // Configure WASI with proper stdio
  auto wasi_config = wasmtime::WasiConfig();
  wasi_config.inherit_stdin();
  wasi_config.inherit_stdout();
  wasi_config.inherit_stderr();

  // Set WASI configuration on store context
  store_.context().set_wasi(std::move(wasi_config)).unwrap();

  linker_.define_wasi().unwrap();

  // Add stub functions for C++ runtime functions that might be needed
  defineStubImports();

  // Read and compile the WASM file
  std::vector<uint8_t> wasm_bytes = readBinaryFile(module_path);
  wasmtime::Span<uint8_t> wasm_span(wasm_bytes.data(), wasm_bytes.size());
  module_ = wasmtime::Module::compile(engine_, wasm_span).unwrap();

  // Instantiate the module
  instance_ = linker_.instantiate(store_, *module_).unwrap();

  // Get memory export for future use
  auto memory_export = instance_->get(store_, "memory");
  if (!memory_export || !std::holds_alternative<wasmtime::Memory>(*memory_export))
  {
    throw std::runtime_error("Memory export not found");
  }

  memory_ = std::get<wasmtime::Memory>(*memory_export);

  // Try to initialize global constructors for C++ iostream support
  // First try _initialize (for reactor modules), then _start (for command modules)
  bool initialized = false;

  try
  {
    auto init_func = getFunc("_initialize");
    init_func.call(store_, {}).unwrap();
    initialized = true;
  }
  catch (const std::exception&)
  {
    // _initialize not found, this is normal for command modules
  }

  if (!initialized)
  {
    try
    {
      auto ctors_func = getFunc("__wasm_call_ctors");
      ctors_func.call(store_, {}).unwrap();
      initialized = true;
    }
    catch (const std::exception&)
    {
      // __wasm_call_ctors not found
    }
  }
}

wasmtime::Func WasmRuntime::getFunc(const std::string& name)
{
  auto func_export = instance_->get(store_, name);
  if (!func_export || !std::holds_alternative<wasmtime::Func>(*func_export))
  {
    throw std::runtime_error(name + " function not found");
  }
  return std::get<wasmtime::Func>(*func_export);
}

std::string WasmRuntime::wasmValueToString(const wasmtime::Val& val)
{
  // Check if the value is an i32 (pointer)
  if (val.kind() != wasmtime::ValKind::I32)
  {
    throw std::runtime_error("Value is not an i32 pointer");
  }

  const int32_t str_ptr = val.i32();
  if (str_ptr == 0)
  {
    throw std::runtime_error("Null pointer returned");
  }

  // Get memory data
  const auto data = memory_->data(store_);
  const size_t data_size = data.size();

  if (static_cast<size_t>(str_ptr) >= data_size)
  {
    throw std::runtime_error("String pointer out of bounds");
  }

  // Read the null-terminated string
  const char* manifest_str = reinterpret_cast<const char*>(data.data() + str_ptr);
  const size_t max_len = data_size - str_ptr;
  const size_t str_len = strnlen(manifest_str, max_len);

  if (str_len == max_len)
  {
    throw std::runtime_error("String not null-terminated");
  }
  return std::string(manifest_str, str_len);
}

int32_t WasmRuntime::allocateBuffer(const void* buffer, size_t size)
{
  if (!memory_)
  {
    throw std::runtime_error("Memory not available");
  }

  // Try to get malloc function from WASM module
  auto malloc_func = getFunc("malloc");

  auto results = malloc_func.call(store_, { wasmtime::Val(static_cast<int32_t>(size)) }).unwrap();
  int32_t ptr = results[0].i32();

  if (ptr == 0)
  {
    throw std::runtime_error("WASM malloc returned null");
  }

  // Copy buffer to WASM memory
  auto data = memory_->data(store_);
  if (static_cast<size_t>(ptr + size) > data.size())
  {
    throw std::runtime_error("Allocated memory out of bounds");
  }
  if (buffer)
  {
    std::memcpy(data.data() + ptr, buffer, size);
  }
  return ptr;
}

int32_t WasmRuntime::allocateString(const std::string& str)
{
  return allocateBuffer(str.c_str(), str.size() + 1);
}

void WasmRuntime::freeWasmMemory(int32_t ptr)
{
  // Try to call free function if available
  try
  {
    auto free_func = getFunc("free");
    free_func.call(store_, { wasmtime::Val(ptr) }).unwrap();
  }
  catch (const std::exception&)
  {
    // If free is not available, we can't deallocate
    // This is a limitation of the manual allocation approach
  }
}

// WasmString implementation
WasmString::WasmString(WasmRuntime& runtime, const std::string& str)
  : runtime_(&runtime), ptr_(runtime.allocateString(str))
{
}

WasmString::~WasmString()
{
  if (runtime_ && ptr_ != 0)
  {
    runtime_->freeWasmMemory(ptr_);
  }
}

WasmString::WasmString(WasmString&& other) noexcept : runtime_(other.runtime_), ptr_(other.ptr_)
{
  other.runtime_ = nullptr;
  other.ptr_ = 0;
}

WasmString& WasmString::operator=(WasmString&& other) noexcept
{
  if (this != &other)
  {
    if (runtime_ && ptr_ != 0)
    {
      runtime_->freeWasmMemory(ptr_);
    }
    runtime_ = other.runtime_;
    ptr_ = other.ptr_;
    other.runtime_ = nullptr;
    other.ptr_ = 0;
  }
  return *this;
}

std::string readPluginManifest(WasmRuntime& runtime)
{
  auto manifest_func = runtime.getFunc("pj_plugin_manifest");

  // Call the function (no arguments, returns i32 pointer)
  auto results = manifest_func.call(runtime.store(), {}).unwrap();
  return runtime.wasmValueToString(results[0]);
}
