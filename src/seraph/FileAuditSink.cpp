#include "FileAuditSink.hpp"
#include <chrono>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace Architect::Seraph {

namespace {
    uint64_t GetCurrentTimestampMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
}

FileAuditSink::FileAuditSink(const std::string& filepath) {
    // Open in append mode
    out_stream_.open(filepath, std::ios::out | std::ios::app);
    if (!out_stream_.is_open()) {
        std::cerr << "[FileAuditSink] WARNING: Failed to open or create audit log path: " << filepath << "\n";
    }
}

FileAuditSink::~FileAuditSink() {
    if (out_stream_.is_open()) {
        out_stream_.flush();
        out_stream_.close();
    }
}

void FileAuditSink::AppendJsonLine(const std::string& json_str) {
    if (!out_stream_.is_open()) return;
    
    std::lock_guard<std::mutex> lock(write_mutex_);
    out_stream_ << json_str << "\n";
    out_stream_.flush();
}

void FileAuditSink::LogExecutionStart(const InvocationRequest& request) {
    nlohmann::json record = {
        {"event", "execution_start"},
        {"timestamp_ms", GetCurrentTimestampMs()},
        {"module_name", request.module_name},
        {"function_name", request.function_name},
        {"input_payload_size", nlohmann::json(request.arguments).dump().size()}
    };
    AppendJsonLine(record.dump());
}

void FileAuditSink::LogExecutionSuccess(const InvocationRequest& request, const InvocationResult& result) {
    nlohmann::json record = {
        {"event", "execution_success"},
        {"timestamp_ms", GetCurrentTimestampMs()},
        {"module_name", request.module_name},
        {"function_name", request.function_name},
        {"duration_ms", result.execution_time_ms},
        {"output_size", result.output.size()}
    };
    AppendJsonLine(record.dump());
}

void FileAuditSink::LogExecutionFailure(const InvocationRequest& request, ExecutionError error) {
    nlohmann::json record = {
        {"event", "execution_failure"},
        {"timestamp_ms", GetCurrentTimestampMs()},
        {"module_name", request.module_name},
        {"function_name", request.function_name},
        {"error_code", static_cast<int>(error)},
        {"error_message", GetSemanticFeedback(error)}
    };
    AppendJsonLine(record.dump());
}

void FileAuditSink::LogSystemEvent(const std::string& event_type, const std::string& payload_json) {
    nlohmann::json record = {
        {"event", event_type},
        {"timestamp_ms", GetCurrentTimestampMs()}
    };
    
    try {
        if (!payload_json.empty() && payload_json != "{}") {
            record["details"] = nlohmann::json::parse(payload_json);
        }
    } catch (...) {
        record["details_raw"] = payload_json; // fallback if invalid json
    }
    
    AppendJsonLine(record.dump());
}

} // namespace Architect::Seraph
