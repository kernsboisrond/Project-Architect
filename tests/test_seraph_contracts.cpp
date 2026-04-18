#include "seraph/ExecutorStub.hpp"
#include "seraph/IAuditSink.hpp"
#include "seraph/ExecutionDiagnostics.hpp"
#include "seraph/InvocationTypes.hpp"

#include <iostream>
#include <string>

using namespace Architect::Seraph;

class RecordingAuditSink final : public IAuditSink {
public:
    int start_count{0};
    int success_count{0};
    int failure_count{0};

    void LogExecutionStart(const InvocationRequest&) override {
        start_count++;
    }
    void LogExecutionSuccess(const InvocationRequest&, const InvocationResult&) override {
        success_count++;
    }
    void LogExecutionFailure(const InvocationRequest&, ExecutionError) override {
        failure_count++;
    }
};

int main() {
    std::cout << "--- Testing SERAPH Contracts ---\n";
    
    std::cout << "\n[Test] Semantic Feedback String Mapping\n";
    std::string feedback = GetSemanticFeedback(ExecutionError::UnsupportedModule);
    if (feedback.find("requested module does not exist") == std::string::npos) {
        std::cerr << "FAILED: feedback mapping incorrect.\n";
        return 1;
    }
    std::cout << "PASS: Semantic Feedback\n";

    ExecutorStub stub;
    RecordingAuditSink audit;

    std::cout << "\n[Test] ExecutorStub Capability Deny Path\n";
    InvocationRequest req1;
    req1.module_name = "secret_module";
    req1.function_name = "hack";
    auto res1 = stub.Execute(req1, audit);
    if (res1.has_value() || res1.error() != ExecutionError::CapabilityDenied) {
        std::cerr << "FAILED: Expected CapabilityDenied.\n";
        return 1;
    }
    if (audit.start_count != 1 || audit.failure_count != 1 || audit.success_count != 0) {
        std::cerr << "FAILED: Audit hooks not hit on failure.\n";
        return 1;
    }
    std::cout << "PASS: Capability Deny Path\n";

    std::cout << "\n[Test] ExecutorStub Success Path\n";
    InvocationRequest req2;
    req2.module_name = "echo";
    req2.function_name = "print";
    req2.arguments["message"] = "test payload";
    req2.capabilities.allowed_exports["echo"].push_back("print");
    
    auto res2 = stub.Execute(req2, audit);
    if (!res2.has_value() || !res2->success || res2->output != "test payload") {
        std::cerr << "FAILED: Expected Execute success.\n";
        return 1;
    }
    if (audit.start_count != 2 || audit.failure_count != 1 || audit.success_count != 1) {
        std::cerr << "FAILED: Audit hooks not hit on success.\n";
        return 1;
    }
    std::cout << "PASS: ExecutorStub Success Path\n";

    std::cout << "\nALL SERAPH CONTRACT TESTS PASSED.\n";
    return 0;
}
