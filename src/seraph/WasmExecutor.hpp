#pragma once

#include "IExecutor.hpp"
#include <string>

// Forward declare Wasmtime C API engine to prevent header leak universally
struct wasm_engine_t;

namespace Architect::Seraph {

class WasmExecutor final : public IExecutor {
public:
    explicit WasmExecutor(std::string base_module_dir);
    ~WasmExecutor() override;

    // Disable copy/move to strictly manage the raw FFI pointers
    WasmExecutor(const WasmExecutor&) = delete;
    WasmExecutor& operator=(const WasmExecutor&) = delete;

    [[nodiscard]]
    std::expected<InvocationResult, ExecutionError>
    Execute(const InvocationRequest& request, IAuditSink& audit) const override;

private:
    [[nodiscard]]
    bool PreFlightCheck(const InvocationRequest& request) const;

private:
    std::string base_module_dir_;
    wasm_engine_t* engine_{nullptr};
};

} // namespace Architect::Seraph
