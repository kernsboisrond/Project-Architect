#pragma once

#include "InvocationTypes.hpp"
#include <expected>

namespace Architect::Seraph {

enum class ExecutionError {
    UnsupportedModule,
    UnsupportedFunction,
    CapabilityDenied,
    InvalidArguments
};

class ExecutorStub {
public:
    [[nodiscard]]
    std::expected<InvocationResult, ExecutionError>
    Execute(const InvocationRequest& request) const;

private:
    [[nodiscard]]
    bool PreFlightCheck(const InvocationRequest& request) const;
};

} // namespace Architect::Seraph
