#include "seraph/ExecutorStub.hpp"
#include "seraph/IAuditSink.hpp"
#include <iostream>
#include <unordered_map>

using namespace Architect::Seraph;

void check_error(std::expected<InvocationResult, ExecutionError> result, ExecutionError expected_err, const std::string& test_name) {
    if (result.has_value()) {
        std::cerr << "Test '" << test_name << "' failed: Expected error, but got success!\n";
        exit(1);
    }
    if (result.error() != expected_err) {
        std::cerr << "Test '" << test_name << "' failed: Got wrong error code!\n";
        exit(1);
    }
    std::cout << "Test '" << test_name << "' passed.\n";
}

int main() {
    ExecutorStub executor;
    NoOpAuditSink audit;
    
    std::cout << "\n--- Test 1: Unsupported Module ---\n";
    InvocationRequest req1;
    req1.module_name = "file_system";
    req1.function_name = "read";
    req1.capabilities.allowed_exports["file_system"].push_back("read");
    check_error(executor.Execute(req1, audit), ExecutionError::UnsupportedModule, "Unsupported Module");

    std::cout << "\n--- Test 2: Unsupported Function ---\n";
    InvocationRequest req2;
    req2.module_name = "echo";
    req2.function_name = "format";
    req2.capabilities.allowed_exports["echo"].push_back("format");
    check_error(executor.Execute(req2, audit), ExecutionError::UnsupportedFunction, "Unsupported Function");

    std::cout << "\n--- Test 3: Invalid Arguments (Missing) ---\n";
    InvocationRequest req3;
    req3.module_name = "echo";
    req3.function_name = "print";
    // Grant capability so we hit invalid arguments (after capability check)
    req3.capabilities.allowed_exports["echo"].push_back("print");
    check_error(executor.Execute(req3, audit), ExecutionError::InvalidArguments, "Invalid Arguments");

    std::cout << "\n--- Test 4: Valid Execution ---\n";
    InvocationRequest req4;
    req4.module_name = "echo";
    req4.function_name = "print";
    req4.arguments["message"] = "Hello, Matrix!";
    // Grant capability so we reach valid execution
    req4.capabilities.allowed_exports["echo"].push_back("print");
    
    auto res4 = executor.Execute(req4, audit);
    if (!res4.has_value() || !res4->success) {
        std::cerr << "Test 'Valid Execution' failed: Expected success, got error!\n";
        return 1;
    }
    if (res4->output != "Hello, Matrix!") {
        std::cerr << "Test 'Valid Execution' failed: Got wrong output string '" << res4->output << "'\n";
        return 1;
    }
    std::cout << "Test 'Valid Execution' passed.\n";

    std::cout << "\nExecutor Validation tests passed.\n";
    return 0;
}
