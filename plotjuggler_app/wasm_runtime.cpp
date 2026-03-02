#include "wasm_runtime.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// File reader
// ---------------------------------------------------------------------------

static std::vector<uint8_t> readBinaryFile(const std::string& path)
{
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file)
  {
    throw std::runtime_error("Failed to open WASM file: " + path);
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<uint8_t> buf(size);
  if (!file.read(reinterpret_cast<char*>(buf.data()), size))
  {
    throw std::runtime_error("Failed to read WASM file");
  }
  return buf;
}

// ---------------------------------------------------------------------------
// Stub import callbacks (C-style — cannot capture lambdas)
// ---------------------------------------------------------------------------

struct StubEnv
{
  std::string name;
};

static wasm_trap_t* env_stub_callback(void* env_ptr, const wasm_val_vec_t* /*args*/,
                                      wasm_val_vec_t* results)
{
  const StubEnv* stub = static_cast<const StubEnv*>(env_ptr);

  // Zero-initialise all results
  for (size_t i = 0; i < results->size; i++)
  {
    results->data[i].kind = WASM_I32;
    results->data[i].of.i32 = 0;
  }

  // __cxa_allocate_exception must return a non-null "pointer"
  if (stub->name == "__cxa_allocate_exception" && results->size > 0)
  {
    results->data[0].of.i32 = 1;
  }

  if (stub->name == "__cxa_throw" || stub->name == "abort")
  {
    std::cerr << "Warning: WASM called " << stub->name << "\n";
  }

  return nullptr;
}

static void env_stub_finalizer(void* env_ptr)
{
  delete static_cast<StubEnv*>(env_ptr);
}

// ---------------------------------------------------------------------------
// WasmRuntime constructor / destructor
// ---------------------------------------------------------------------------

WasmRuntime::WasmRuntime(const std::string& module_path)
{
  // 1. Engine + Store
  engine_ = wasm_engine_new();
  if (!engine_)
  {
    throw std::runtime_error("Failed to create WASM engine");
  }
  store_ = wasm_store_new(engine_);
  if (!store_)
  {
    throw std::runtime_error("Failed to create WASM store");
  }

  // 2. WASI environment
  wasi_config_t* wasi_config = wasi_config_new("plotjuggler");
  wasi_config_inherit_stdin(wasi_config);
  wasi_config_inherit_stdout(wasi_config);
  wasi_config_inherit_stderr(wasi_config);

  wasi_env_ = wasi_env_new(store_, wasi_config);
  // wasi_config is consumed by wasi_env_new
  if (!wasi_env_)
  {
    throw std::runtime_error("Failed to create WASI environment");
  }

  // 3. Compile module
  std::vector<uint8_t> raw_bytes = readBinaryFile(module_path);
  wasm_byte_vec_t wasm_bytes;
  wasm_byte_vec_new(&wasm_bytes, raw_bytes.size(),
                    reinterpret_cast<const wasm_byte_t*>(raw_bytes.data()));

  module_ = wasm_module_new(store_, &wasm_bytes);
  wasm_byte_vec_delete(&wasm_bytes);

  if (!module_)
  {
    throw std::runtime_error("Failed to compile WASM module: " + module_path);
  }

  // 4. Resolve imports and instantiate
  buildAndInstantiate();
}

WasmRuntime::~WasmRuntime()
{
  // exports_ holds owned externs (including the borrowed memory_ view)
  wasm_extern_vec_delete(&exports_);
  export_funcs_.clear();

  if (instance_)
  {
    wasm_instance_delete(instance_);
  }

  // Host stub functions (their finalizers clean up StubEnv)
  for (wasm_func_t* f : host_funcs_)
  {
    wasm_func_delete(f);
  }

  if (wasi_env_)
  {
    wasi_env_delete(wasi_env_);
  }
  if (module_)
  {
    wasm_module_delete(module_);
  }
  if (store_)
  {
    wasm_store_delete(store_);
  }
  if (engine_)
  {
    wasm_engine_delete(engine_);
  }
}

// ---------------------------------------------------------------------------
// buildAndInstantiate — resolve imports, create instance, cache exports
// ---------------------------------------------------------------------------

void WasmRuntime::buildAndInstantiate()
{
  // Enumerate the module's import requirements (module name + function name)
  wasm_importtype_vec_t import_types;
  wasm_module_imports(module_, &import_types);

  // Get WASI imports by name (wasi_get_imports is broken in wasmer 6.x)
  wasmer_named_extern_vec_t wasi_named;
  wasmer_named_extern_vec_new_empty(&wasi_named);
  wasi_get_unordered_imports(wasi_env_, module_, &wasi_named);

  // Build (module::name) → extern lookup map from the named externs
  std::unordered_map<std::string, const wasm_extern_t*> wasi_map;
  for (size_t i = 0; i < wasi_named.size; i++)
  {
    const wasmer_named_extern_t* ne = wasi_named.data[i];
    const wasm_name_t* m = wasmer_named_extern_module(ne);
    const wasm_name_t* n = wasmer_named_extern_name(ne);
    std::string key = std::string(m->data, m->size) + "::" + std::string(n->data, n->size);
    wasi_map[key] = wasmer_named_extern_unwrap(ne);
  }

  // Build a combined import array (borrowed pointers — we never call
  // wasm_extern_vec_delete on this; wasi_imports owns the WASI externs and
  // host_funcs_ owns the stub externs)
  std::vector<wasm_extern_t*> imports_ptrs(import_types.size, nullptr);

  for (size_t i = 0; i < import_types.size; i++)
  {
    const wasm_name_t* mod_name = wasm_importtype_module(import_types.data[i]);
    const wasm_name_t* fn_name = wasm_importtype_name(import_types.data[i]);
    std::string mod_str(mod_name->data, mod_name->size);
    std::string fn_str(fn_name->data, fn_name->size);
    std::string key = mod_str + "::" + fn_str;

    // Use WASI extern if available
    auto it = wasi_map.find(key);
    if (it != wasi_map.end())
    {
      imports_ptrs[i] = const_cast<wasm_extern_t*>(it->second);
      continue;
    }

    // Otherwise create a no-op stub (for "env" and any other unresolved imports)
    if (mod_str != "env")
    {
      std::cerr << "Warning: unresolved import " << key << " — providing no-op stub\n";
    }

    const wasm_externtype_t* ext_type = wasm_importtype_type(import_types.data[i]);
    const wasm_functype_t* func_type = wasm_externtype_as_functype_const(ext_type);
    if (!func_type)
    {
      std::cerr << "Warning: import " << key << " is not a function — skipping\n";
      continue;
    }

    auto* senv = new StubEnv{ fn_str };
    wasm_func_t* stub =
        wasm_func_new_with_env(store_, func_type, env_stub_callback, senv, env_stub_finalizer);
    host_funcs_.push_back(stub);
    imports_ptrs[i] = wasm_func_as_extern(stub);
  }

  // Hand the combined array to wasm_instance_new (it does not take ownership)
  wasm_extern_vec_t all_imports;
  all_imports.size = imports_ptrs.size();
  all_imports.data = imports_ptrs.data();

  wasm_trap_t* trap = nullptr;
  instance_ = wasm_instance_new(store_, module_, &all_imports, &trap);

  // Clean up: instance has internalised what it needs from imports
  wasmer_named_extern_vec_delete(&wasi_named);
  wasm_importtype_vec_delete(&import_types);
  // imports_ptrs is a local vector — no further cleanup needed

  if (trap)
  {
    wasm_message_t msg = WASM_EMPTY_VEC;
    wasm_trap_message(trap, &msg);
    std::string err(msg.data, msg.size);
    wasm_byte_vec_delete(&msg);
    wasm_trap_delete(trap);
    throw std::runtime_error("WASM instantiation trap: " + err);
  }
  if (!instance_)
  {
    throw std::runtime_error("Failed to instantiate WASM module");
  }

  // Tell WASI about the instance (sets up memory pointer etc.)
  wasi_env_initialize_instance(wasi_env_, store_, instance_);

  // Cache all exports
  wasm_instance_exports(instance_, &exports_);

  wasm_exporttype_vec_t export_types;
  wasm_module_exports(module_, &export_types);

  for (size_t i = 0; i < export_types.size && i < exports_.size; i++)
  {
    wasm_extern_t* ext = exports_.data[i];
    if (!ext)
    {
      continue;
    }

    const wasm_name_t* name = wasm_exporttype_name(export_types.data[i]);
    std::string name_str(name->data, name->size);

    if (wasm_extern_kind(ext) == WASM_EXTERN_FUNC)
    {
      export_funcs_[name_str] = wasm_extern_as_func(ext);
    }
    else if (wasm_extern_kind(ext) == WASM_EXTERN_MEMORY && !memory_)
    {
      memory_ = wasm_extern_as_memory(ext);
    }
  }

  wasm_exporttype_vec_delete(&export_types);

  if (!memory_)
  {
    throw std::runtime_error("WASM module does not export 'memory'");
  }

  // Run global constructors / reactor init if present
  for (const char* init_name : { "_initialize", "__wasm_call_ctors" })
  {
    auto it = export_funcs_.find(init_name);
    if (it != export_funcs_.end())
    {
      callFunc(it->second, {});
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

wasm_func_t* WasmRuntime::getFunc(const std::string& name)
{
  auto it = export_funcs_.find(name);
  if (it == export_funcs_.end())
  {
    throw std::runtime_error(name + " function not found in WASM exports");
  }
  return it->second;
}

std::vector<int32_t> WasmRuntime::callFunc(wasm_func_t* func, const std::vector<int32_t>& args)
{
  // Build args vec
  wasm_val_vec_t args_vec;
  wasm_val_vec_new_uninitialized(&args_vec, args.size());
  for (size_t i = 0; i < args.size(); i++)
  {
    args_vec.data[i].kind = WASM_I32;
    args_vec.data[i].of.i32 = args[i];
  }

  // Get result arity from the function type
  const wasm_functype_t* ft = wasm_func_type(func);
  size_t n_results = wasm_functype_results(ft)->size;

  wasm_val_vec_t results_vec;
  wasm_val_vec_new_uninitialized(&results_vec, n_results);

  wasm_trap_t* trap = wasm_func_call(func, &args_vec, &results_vec);

  wasm_val_vec_delete(&args_vec);

  if (trap)
  {
    wasm_message_t msg = WASM_EMPTY_VEC;
    wasm_trap_message(trap, &msg);
    std::string err(msg.data, msg.size);
    wasm_byte_vec_delete(&msg);
    wasm_trap_delete(trap);
    throw std::runtime_error("WASM function call trap: " + err);
  }

  std::vector<int32_t> out(n_results);
  for (size_t i = 0; i < n_results; i++)
  {
    out[i] = results_vec.data[i].of.i32;
  }
  wasm_val_vec_delete(&results_vec);
  return out;
}

std::string WasmRuntime::wasmValueToString(int32_t str_ptr)
{
  if (str_ptr == 0)
  {
    throw std::runtime_error("Null pointer returned from WASM");
  }

  const size_t mem_size = wasm_memory_data_size(memory_);
  if (static_cast<size_t>(str_ptr) >= mem_size)
  {
    throw std::runtime_error("String pointer out of WASM memory bounds");
  }

  const char* str = reinterpret_cast<const char*>(wasm_memory_data(memory_)) + str_ptr;
  const size_t max_len = mem_size - str_ptr;
  const size_t len = strnlen(str, max_len);

  if (len == max_len)
  {
    throw std::runtime_error("WASM string is not null-terminated");
  }
  return std::string(str, len);
}

int32_t WasmRuntime::allocateBuffer(const void* buffer, size_t size)
{
  auto results = callFunc(getFunc("malloc"), { static_cast<int32_t>(size) });
  int32_t ptr = results[0];
  if (ptr == 0)
  {
    throw std::runtime_error("WASM malloc returned null");
  }

  const size_t mem_size = wasm_memory_data_size(memory_);
  if (static_cast<size_t>(ptr + size) > mem_size)
  {
    throw std::runtime_error("Allocated WASM memory out of bounds");
  }

  if (buffer)
  {
    std::memcpy(wasm_memory_data(memory_) + ptr, buffer, size);
  }
  return ptr;
}

int32_t WasmRuntime::allocateString(const std::string& str)
{
  return allocateBuffer(str.c_str(), str.size() + 1);
}

void WasmRuntime::freeWasmMemory(int32_t ptr)
{
  try
  {
    callFunc(getFunc("free"), { ptr });
  }
  catch (const std::exception&)
  {
    // free not available — ignore
  }
}

// ---------------------------------------------------------------------------
// WasmString
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// readPluginManifest
// ---------------------------------------------------------------------------

std::string readPluginManifest(WasmRuntime& runtime)
{
  auto results = runtime.callFunc(runtime.getFunc("pj_plugin_manifest"), {});
  return runtime.wasmValueToString(results[0]);
}
