#include "ExecutorStub.hpp"

#include <chrono>

namespace Architect::Seraph {

bool ExecutorStub::PreFlightCheck(const InvocationRequest& request) const {
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
ExecutorStub::Execute(const InvocationRequest& request, IAuditSink& audit) const {
    audit.LogExecutionStart(request);
    const auto start_time = std::chrono::steady_clock::now();

    if (!PreFlightCheck(request)) {
        audit.LogExecutionFailure(request, ExecutionError::CapabilityDenied);
        return std::unexpected(ExecutionError::CapabilityDenied);
    }

    if (request.module_name != "echo") {
        audit.LogExecutionFailure(request, ExecutionError::UnsupportedModule);
        return std::unexpected(ExecutionError::UnsupportedModule);
    }

    if (request.function_name != "print") {
        audit.LogExecutionFailure(request, ExecutionError::UnsupportedFunction);
        return std::unexpected(ExecutionError::UnsupportedFunction);
    }

    const auto it = request.arguments.find("message");
    if (it == request.arguments.end()) {
        audit.LogExecutionFailure(request, ExecutionError::InvalidArguments);
        return std::unexpected(ExecutionError::InvalidArguments);
    }

    InvocationResult result;
    result.success = true;
    result.output = it->second;
    result.status_code = 0;
    
    const auto end_time = std::chrono::steady_clock::now();
    result.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    audit.LogExecutionSuccess(request, result);
    return result;
}

} // namespace Architect::Seraph
