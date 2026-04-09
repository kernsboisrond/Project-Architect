#include "ExecutorStub.hpp"

namespace Architect::Seraph {

bool ExecutorStub::PreFlightCheck(const InvocationRequest& request) const {
    if (request.module_name.empty() || request.function_name.empty()) {
        return false;
    }

    if (request.capabilities.allow_process_spawn ||
        request.capabilities.allow_fs_write ||
        request.capabilities.allow_network) {
        return false;
    }

    return true;
}

std::expected<InvocationResult, ExecutionError>
ExecutorStub::Execute(const InvocationRequest& request) const {
    if (!PreFlightCheck(request)) {
        return std::unexpected(ExecutionError::CapabilityDenied);
    }

    if (request.module_name != "echo") {
        return std::unexpected(ExecutionError::UnsupportedModule);
    }

    if (request.function_name != "print") {
        return std::unexpected(ExecutionError::UnsupportedFunction);
    }

    const auto it = request.arguments.find("message");
    if (it == request.arguments.end()) {
        return std::unexpected(ExecutionError::InvalidArguments);
    }

    InvocationResult result;
    result.success = true;
    result.output = it->second;
    return result;
}

} // namespace Architect::Seraph
