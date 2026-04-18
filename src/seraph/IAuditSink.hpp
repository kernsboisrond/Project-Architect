#pragma once

#include "InvocationTypes.hpp"
#include "ExecutionDiagnostics.hpp"

namespace Architect::Seraph {

class IAuditSink {
public:
    virtual ~IAuditSink() = default;

    virtual void LogExecutionStart(const InvocationRequest& request) = 0;
    virtual void LogExecutionSuccess(const InvocationRequest& request, const InvocationResult& result) = 0;
    virtual void LogExecutionFailure(const InvocationRequest& request, ExecutionError error) = 0;
};

class NoOpAuditSink final : public IAuditSink {
public:
    void LogExecutionStart(const InvocationRequest&) override {}
    void LogExecutionSuccess(const InvocationRequest&, const InvocationResult&) override {}
    void LogExecutionFailure(const InvocationRequest&, ExecutionError) override {}
};

} // namespace Architect::Seraph
