#include "seraph/WasmExecutor.hpp"
#include "seraph/IAuditSink.hpp"
#include "seraph/ExecutionDiagnostics.hpp"

#include <iostream>

using namespace Architect::Seraph;

class MockAudit : public IAuditSink {
public:
    void LogExecutionStart(const InvocationRequest&) override {}
    void LogExecutionSuccess(const InvocationRequest&, const InvocationResult&) override {}
    void LogExecutionFailure(const InvocationRequest&, ExecutionError) override {}
};

int main() {
    WasmExecutor executor("tests/fixtures");
    MockAudit audit;

    InvocationRequest req;
    req.module_name = "echo";
    req.function_name = "print";
    req.arguments["foo"] = "bar"; // JSON length > 0
    req.capabilities.allowed_exports["echo"].push_back("print");

    auto result = executor.Execute(req, audit);
    if (!result.has_value()) {
        std::cerr << "WasmExecutor failed: " << static_cast<int>(result.error()) << "\n";
        return 1;
    }

    std::cout << "WasmExecutor Success!\n";
    std::cout << "Output: " << result->output << "\n";
    
    // Validate that it echoed exactly the JSON string passed in
    if (result->output != "{\"foo\":\"bar\"}") {
        std::cerr << "Failed: unexpected output: " << result->output << "\n";
        return 1;
    }

    // Phase 9D: One Failure-Path Wasm Test (Directory Traversal blocking)
    InvocationRequest bad_req;
    bad_req.module_name = "../evil";
    bad_req.function_name = "hack";
    bad_req.capabilities.allowed_exports["../evil"].push_back("hack");
    
    auto bad_result = executor.Execute(bad_req, audit);
    if (bad_result.has_value()) {
        std::cerr << "WasmExecutor unexpectedly passed traversal block.\n";
        return 1;
    }

    if (bad_result.error() != ExecutionError::UnsupportedModule) {
        std::cerr << "WasmExecutor traversal failed but yielded wrong semantic error.\n";
        return 1;
    }

    std::cout << "Traversal block successfully trapped path tampering!\n";
    return 0;
}
