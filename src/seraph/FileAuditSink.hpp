#pragma once

#include "IAuditSink.hpp"
#include <fstream>
#include <mutex>
#include <string>

namespace Architect::Seraph {

class FileAuditSink final : public IAuditSink {
public:
    explicit FileAuditSink(const std::string& filepath);
    ~FileAuditSink() override;

    // Delete copy/move
    FileAuditSink(const FileAuditSink&) = delete;
    FileAuditSink& operator=(const FileAuditSink&) = delete;

    void LogExecutionStart(const InvocationRequest& request) override;
    void LogExecutionSuccess(const InvocationRequest& request, const InvocationResult& result) override;
    void LogExecutionFailure(const InvocationRequest& request, ExecutionError error) override;
    void LogSystemEvent(const std::string& event_type, const std::string& payload_json = "{}") override;

private:
    void AppendJsonLine(const std::string& json_str);

private:
    std::ofstream out_stream_;
    std::mutex write_mutex_;
};

} // namespace Architect::Seraph
