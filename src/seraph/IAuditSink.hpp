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
    
    // Phase 13: Boot and Status Events
    virtual void LogSystemEvent(const std::string& event_type, const std::string& payload_json = "{}") = 0;
};

class NoOpAuditSink final : public IAuditSink {
public:
    void LogExecutionStart(const InvocationRequest&) override {}
    void LogExecutionSuccess(const InvocationRequest&, const InvocationResult&) override {}
    void LogExecutionFailure(const InvocationRequest&, ExecutionError) override {}
    void LogSystemEvent(const std::string&, const std::string&) override {}
};

} // namespace Architect::Seraph
