#pragma once

#include "IExecutor.hpp"
#include "ModuleRegistry.hpp"
#include <memory>
#include <string>
#include <unordered_map>

// Forward declare Wasmtime C API engine to prevent header leak universally
struct wasm_engine_t;
struct wasmtime_module;
typedef struct wasmtime_module wasmtime_module_t;

namespace Architect::Seraph {

struct RuntimeLimits {
    uint32_t max_input_size{64 * 1024}; // 64 KB
    uint32_t max_output_size{64 * 1024};
    uint64_t max_fuel{100'000'000};     // 100M instructions
};

class WasmExecutor final : public IExecutor {
public:
    explicit WasmExecutor(std::shared_ptr<ModuleRegistry> registry,
                          const RuntimeLimits& limits = RuntimeLimits{});
    ~WasmExecutor() override;

    // Disable copy/move to strictly manage the raw FFI pointers
    WasmExecutor(const WasmExecutor&) = delete;
    WasmExecutor& operator=(const WasmExecutor&) = delete;

    [[nodiscard]]
    std::expected<InvocationResult, ExecutionError>
    Execute(const InvocationRequest& request, IAuditSink& audit) const override;

    [[nodiscard]]
    size_t InspectCacheCount() const;

    void ClearCache();

private:
    [[nodiscard]]
    bool PreFlightCheck(const InvocationRequest& request) const;

private:
    std::shared_ptr<ModuleRegistry> registry_;
    RuntimeLimits limits_;
    wasm_engine_t* engine_{nullptr};
    mutable std::unordered_map<std::string, wasmtime_module_t*> cached_modules_;
};

} // namespace Architect::Seraph
