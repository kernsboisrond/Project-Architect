#pragma once

#include "InvocationTypes.hpp"
#include "ExecutionDiagnostics.hpp"
#include "IAuditSink.hpp"

#include <expected>

namespace Architect::Seraph {

class IExecutor {
public:
    virtual ~IExecutor() = default;

    [[nodiscard]]
    virtual std::expected<InvocationResult, ExecutionError>
    Execute(const InvocationRequest& request, IAuditSink& audit) const = 0;
};

} // namespace Architect::Seraph
