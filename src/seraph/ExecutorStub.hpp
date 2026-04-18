#pragma once

#include "IExecutor.hpp"

namespace Architect::Seraph {

class ExecutorStub final : public IExecutor {
public:
    [[nodiscard]]
    std::expected<InvocationResult, ExecutionError>
    Execute(const InvocationRequest& request, IAuditSink& audit) const override;

private:
    [[nodiscard]]
    bool PreFlightCheck(const InvocationRequest& request) const;
};

} // namespace Architect::Seraph
