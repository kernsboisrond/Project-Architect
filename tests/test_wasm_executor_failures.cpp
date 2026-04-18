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

void run_test(WasmExecutor& executor, MockAudit& audit, const std::string& module_name, ExecutionError expected_error) {
    InvocationRequest req;
    req.module_name = module_name;
    req.function_name = "print";
    req.arguments["foo"] = "bar_10_bytes"; // Ensure payload > 0 to trigger alloc checks 
    req.capabilities.allowed_exports[module_name].push_back("print");

    auto result = executor.Execute(req, audit);
    if (result.has_value()) {
        std::cerr << "[FAIL] Expected failure for module " << module_name << " but execution succeeded!\n";
        exit(1);
    } else if (result.error() != expected_error) {
        std::cerr << "[FAIL] Module " << module_name 
                  << " failed with wrong ExecutionError. Expected " << static_cast<int>(expected_error) 
                  << ", got " << static_cast<int>(result.error()) << "\n";
        exit(1);
    }
    std::cout << "[PASS] Module " << module_name << " failed with correct error.\n";
}

int main() {
    auto registry = std::make_shared<ModuleRegistry>();
    if (!registry->LoadManifest("tests/fixtures/manifest.json").has_value()) {
        std::cerr << "Failed to construct failure test registry from fixtures.\n";
        return 1;
    }

    WasmExecutor executor(registry);
    MockAudit audit;

    run_test(executor, audit, "bad", ExecutionError::WasmCompileError);
    run_test(executor, audit, "missing_alloc", ExecutionError::MissingExport);
    run_test(executor, audit, "input_oob", ExecutionError::AllocatorError);
    run_test(executor, audit, "trap", ExecutionError::GuestTrap);
    run_test(executor, audit, "invalid_ptr", ExecutionError::InvalidMemoryBounds);
    
    // Phase 11 specific tests natively bridged over to the Registry!
    run_test(executor, audit, "untrusted_mod", ExecutionError::UntrustedModule);
    run_test(executor, audit, "unregistered_mod", ExecutionError::CapabilityDenied); // Blocked natively traversing PreFlight.

    std::cout << "\nAll Negative-Path Wasm tests passed successfully!\n";
    return 0;
}
