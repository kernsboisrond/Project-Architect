#include "WasmExecutor.hpp"
#include "SeraphGuestAbi.hpp"

#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <nlohmann/json.hpp>
#include <wasmtime.h>

namespace Architect::Seraph {

WasmExecutor::WasmExecutor(std::string base_module_dir)
    : base_module_dir_(std::move(base_module_dir)) {
    wasm_config_t* config = wasm_config_new();
    engine_ = wasm_engine_new_with_config(config);
}

WasmExecutor::~WasmExecutor() {
    for (auto& [name, module] : cached_modules_) {
        wasmtime_module_delete(module);
    }
    cached_modules_.clear();
    
    if (engine_) {
        wasm_engine_delete(engine_);
    }
}

bool WasmExecutor::PreFlightCheck(const InvocationRequest& request) const {
    if (request.module_name.empty() || request.function_name.empty()) {
        return false;
    }

    const auto& exports = request.capabilities.allowed_exports;
    auto it = exports.find(request.module_name);
    if (it == exports.end()) {
        return false;
    }

    bool func_allowed = false;
    for (const auto& func : it->second) {
        if (func == request.function_name) {
            func_allowed = true;
            break;
        }
    }

    return func_allowed;
}

std::expected<InvocationResult, ExecutionError>
WasmExecutor::Execute(const InvocationRequest& request, IAuditSink& audit) const {
    audit.LogExecutionStart(request);
    const auto start_time = std::chrono::steady_clock::now();

    if (!PreFlightCheck(request)) {
        audit.LogExecutionFailure(request, ExecutionError::CapabilityDenied);
        return std::unexpected(ExecutionError::CapabilityDenied);
    }

    // Harden module_name to firmly prevent traversal or invalid names
    static const std::regex module_name_regex("^[a-zA-Z0-9_-]+$");
    if (!std::regex_match(request.module_name, module_name_regex)) {
        audit.LogExecutionFailure(request, ExecutionError::InvalidModule);
        return std::unexpected(ExecutionError::InvalidModule);
    }
    
    wasmtime_module_t* module = nullptr;
    
    // Phase 10C: Check Cache
    if (auto it = cached_modules_.find(request.module_name); it != cached_modules_.end()) {
        module = it->second;
    } else {
        std::string wat_path = base_module_dir_ + "/" + request.module_name + ".wat";
        std::string wasm_path = base_module_dir_ + "/" + request.module_name + ".wasm";

        wasm_byte_vec_t wasm;
        wasm_byte_vec_new_empty(&wasm);

        std::ifstream wat_file(wat_path, std::ios::binary);
        if (wat_file.is_open()) {
            std::stringstream buffer;
            buffer << wat_file.rdbuf();
            std::string wat_content = buffer.str();

            wasmtime_error_t* err = wasmtime_wat2wasm(wat_content.data(), wat_content.size(), &wasm);
            if (err) {
                wasmtime_error_delete(err);
                audit.LogExecutionFailure(request, ExecutionError::WasmCompileError);
                return std::unexpected(ExecutionError::WasmCompileError);
            }
        } else {
            std::ifstream wasm_file(wasm_path, std::ios::binary);
            if (!wasm_file.is_open()) {
                audit.LogExecutionFailure(request, ExecutionError::FileNotFound);
                return std::unexpected(ExecutionError::FileNotFound);
            }
            
            wasm_file.seekg(0, std::ios::end);
            size_t size = wasm_file.tellg();
            wasm_file.seekg(0, std::ios::beg);
            
            wasm_byte_vec_new_uninitialized(&wasm, size);
            wasm_file.read(reinterpret_cast<char*>(wasm.data), size);
        }
        
        wasmtime_error_t* error = wasmtime_module_new(engine_, (uint8_t*)wasm.data, wasm.size, &module);
        wasm_byte_vec_delete(&wasm);

        if (error) {
            wasmtime_error_delete(error);
            audit.LogExecutionFailure(request, ExecutionError::ModuleLoadError);
            return std::unexpected(ExecutionError::ModuleLoadError);
        }
        
        cached_modules_[request.module_name] = module;
    }
    
    wasmtime_store_t* store = wasmtime_store_new(engine_, nullptr, nullptr);
    wasmtime_context_t* context = wasmtime_store_context(store);

    wasmtime_linker_t* linker = wasmtime_linker_new(engine_);
    wasmtime_instance_t instance;
    wasm_trap_t* trap = nullptr;

    wasmtime_error_t* error = wasmtime_linker_instantiate(linker, context, module, &instance, &trap);
    if (error || trap) {
        if (error) wasmtime_error_delete(error);
        if (trap) wasm_trap_delete(trap);
        wasmtime_linker_delete(linker);
        wasmtime_store_delete(store);
        audit.LogExecutionFailure(request, ExecutionError::FunctionCallError);
        return std::unexpected(ExecutionError::FunctionCallError);
    }

    // ABI Resolution
    wasmtime_extern_t ext_alloc;
    bool has_alloc = wasmtime_instance_export_get(context, &instance, std::string(Abi::kExportAlloc).c_str(), Abi::kExportAlloc.size(), &ext_alloc);
    
    wasmtime_extern_t ext_free;
    bool has_free = wasmtime_instance_export_get(context, &instance, std::string(Abi::kExportFree).c_str(), Abi::kExportFree.size(), &ext_free);
    
    wasmtime_extern_t ext_memory;
    bool has_memory = wasmtime_instance_export_get(context, &instance, std::string(Abi::kExportMemory).c_str(), Abi::kExportMemory.size(), &ext_memory);
    
    wasmtime_extern_t ext_func;
    bool has_func = wasmtime_instance_export_get(context, &instance, request.function_name.c_str(), request.function_name.size(), &ext_func);

    if (!has_alloc || ext_alloc.kind != WASMTIME_EXTERN_FUNC ||
        !has_free || ext_free.kind != WASMTIME_EXTERN_FUNC ||
        !has_memory || ext_memory.kind != WASMTIME_EXTERN_MEMORY ||
        !has_func || ext_func.kind != WASMTIME_EXTERN_FUNC) {
        wasmtime_linker_delete(linker);
        wasmtime_store_delete(store);
        audit.LogExecutionFailure(request, ExecutionError::MissingExport);
        return std::unexpected(ExecutionError::MissingExport);
    }

    // Cleanup helper handling Wasm exports gracefully upon Host aborts.
    auto cleanup_and_free = [&](std::uint32_t ptr, std::uint32_t len) {
        wasmtime_val_t free_args[2] = {
            {.kind = WASMTIME_I32, .of = {.i32 = static_cast<int32_t>(ptr)}},
            {.kind = WASMTIME_I32, .of = {.i32 = static_cast<int32_t>(len)}}
        };
        wasm_trap_t* free_trap = nullptr;
        wasmtime_error_t* free_err = wasmtime_func_call(context, &ext_free.of.func, free_args, 2, nullptr, 0, &free_trap);
        if (free_err) wasmtime_error_delete(free_err);
        if (free_trap) wasm_trap_delete(free_trap);
    };

    // Step 1: Memory Allocation
    std::string payload_json = nlohmann::json(request.arguments).dump();
    wasmtime_val_t alloc_args[1] = {{.kind = WASMTIME_I32, .of = {.i32 = static_cast<int32_t>(payload_json.size())}}};
    wasmtime_val_t alloc_results[1] = {};

    error = wasmtime_func_call(context, &ext_alloc.of.func, alloc_args, 1, alloc_results, 1, &trap);
    if (error || trap) {
        ExecutionError f_err = trap ? ExecutionError::GuestTrap : ExecutionError::FunctionCallError;
        if (error) wasmtime_error_delete(error);
        if (trap) wasm_trap_delete(trap);
        wasmtime_linker_delete(linker);
        wasmtime_store_delete(store);
        audit.LogExecutionFailure(request, f_err);
        return std::unexpected(f_err);
    }

    uint32_t payload_ptr = alloc_results[0].of.i32;

    // Step 2: Write Memory (Input Safety)
    uint8_t* memory_data = wasmtime_memory_data(context, &ext_memory.of.memory);
    size_t memory_size = wasmtime_memory_data_size(context, &ext_memory.of.memory);

    uint64_t safe_payload_ptr = payload_ptr;
    uint64_t safe_payload_size = payload_json.size();

    if (safe_payload_ptr + safe_payload_size > static_cast<uint64_t>(memory_size)) {
        cleanup_and_free(payload_ptr, payload_json.size());
        wasmtime_linker_delete(linker);
        wasmtime_store_delete(store);
        audit.LogExecutionFailure(request, ExecutionError::AllocatorError);
        return std::unexpected(ExecutionError::AllocatorError);
    }

    if (safe_payload_size > 0) {
        std::memcpy(memory_data + payload_ptr, payload_json.data(), payload_json.size());
    }

    // Step 3: Function Invocation
    wasmtime_val_t func_args[2] = {
        {.kind = WASMTIME_I32, .of = {.i32 = static_cast<int32_t>(payload_ptr)}},
        {.kind = WASMTIME_I32, .of = {.i32 = static_cast<int32_t>(payload_json.size())}}
    };
    wasmtime_val_t func_results[1] = {};

    error = wasmtime_func_call(context, &ext_func.of.func, func_args, 2, func_results, 1, &trap);
    if (error || trap) {
        ExecutionError f_err = trap ? ExecutionError::GuestTrap : ExecutionError::FunctionCallError;
        cleanup_and_free(payload_ptr, payload_json.size()); // Clean up injected payload allocated bytes
        if (error) wasmtime_error_delete(error);
        if (trap) wasm_trap_delete(trap);
        wasmtime_linker_delete(linker);
        wasmtime_store_delete(store);
        audit.LogExecutionFailure(request, f_err);
        return std::unexpected(f_err);
    }

    uint64_t raw_result = func_results[0].of.i64;
    uint32_t result_ptr = Abi::UnpackPtr(raw_result);
    uint32_t result_len = Abi::UnpackLen(raw_result);

    // Step 4: Read Result (Output Safety)
    memory_data = wasmtime_memory_data(context, &ext_memory.of.memory);
    memory_size = wasmtime_memory_data_size(context, &ext_memory.of.memory);

    uint64_t safe_result_ptr = result_ptr;
    uint64_t safe_result_len = result_len;
    
    InvocationResult result;
    if (safe_result_ptr + safe_result_len <= static_cast<uint64_t>(memory_size)) {
        result.success = true;
        result.output = std::string(reinterpret_cast<const char*>(memory_data + result_ptr), result_len);
        result.status_code = 0;
    } else {
        result.success = false;
    }

    // Step 5: Free Memory Output dynamically
    cleanup_and_free(result_ptr, result_len);
    
    // Fallback error returns executing cleanly after safe guest teardown occurred gracefully inside Host logic!
    if (!result.success) {
        wasmtime_linker_delete(linker);
        wasmtime_store_delete(store);
        audit.LogExecutionFailure(request, ExecutionError::InvalidMemoryBounds);
        return std::unexpected(ExecutionError::InvalidMemoryBounds);
    }

    wasmtime_linker_delete(linker);
    wasmtime_store_delete(store);

    const auto end_time = std::chrono::steady_clock::now();
    result.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    audit.LogExecutionSuccess(request, result);
    return result;
}

} // namespace Architect::Seraph
