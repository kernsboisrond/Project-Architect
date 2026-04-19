#include "seraph/FileAuditSink.hpp"
#include "seraph/WasmExecutor.hpp"
#include "seraph/ModuleRegistry.hpp"
#include "warden/WardenEngine.hpp"
#include "warden/MockBrainBackend.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cassert>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace Architect::Warden;
using namespace Architect::Seraph;

void AssertContains(const std::string& str, const std::string& substr) {
    if (str.find(substr) == std::string::npos) {
        std::cerr << "Assertion failed: expected \n" << str << "\n to contain \n" << substr << "\n";
        std::exit(1);
    }
}

int main() {
    std::cout << "Running Operational Harness Integration Test...\n";

    std::filesystem::path audit_file = "test_audit_harness.jsonl";
    if (std::filesystem::exists(audit_file)) {
        std::filesystem::remove(audit_file);
    }

    {
        FileAuditSink audit{audit_file.string()};
        audit.LogSystemEvent("boot_start");

        auto registry = std::make_shared<ModuleRegistry>();
        auto load_rc = registry->LoadManifest("tests/fixtures/manifest.json");
        if (!load_rc.has_value()) {
            std::cerr << "Failed to load manifest!\n";
            return 1;
        }

        WasmExecutor executor(registry);
        
        auto diag = registry->GetDiagnostics(executor.InspectCacheCount());
        assert(diag.trusted_modules > 0);
        
        nlohmann::json boot_reg = {
            {"trusted", diag.trusted_modules},
            {"policy_exports", diag.policy_exports_count}
        };
        audit.LogSystemEvent("boot_registry_loaded", boot_reg.dump());

        auto brain = std::make_unique<MockBrainBackend>();
        Engine warden(std::move(brain));
        
        // Mock output gives an InvokeSeraph for echo::print
        Architect::Core::AgentContext ctx{};
        ctx.current_stimulus = "Run operational harness validation.";
        auto result = warden.EnforceCognition(ctx);
        assert(result.has_value());
        
        auto intent = result->intent;
        assert(intent.index() == 2);
        
        auto invoke_req = std::get<Architect::Warden::InvokeSeraph>(intent);
        
        InvocationRequest s_req;
        s_req.module_name = invoke_req.target_wasm_module;
        s_req.function_name = invoke_req.target_function;
        s_req.arguments = invoke_req.arguments;
        
        auto exec_res = executor.Execute(s_req, audit);
        assert(exec_res.has_value());
        assert(exec_res->success == true);
        
        // Let's do a cache check after first execution. Cache count should be 1.
        assert(executor.InspectCacheCount() == 1);
        
        nlohmann::json stat_j = {
            {"trusted", diag.trusted_modules},
            {"cached", executor.InspectCacheCount()}
        };
        audit.LogSystemEvent("status_snapshot", stat_j.dump());
    }

    // Now verify the generated jsonl log file
    std::ifstream ifs(audit_file);
    assert(ifs.is_open());
    
    std::string line;
    int line_count = 0;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        auto j = nlohmann::json::parse(line);
        assert(j.contains("event"));
        assert(j.contains("timestamp_ms"));
        
        std::string ev = j["event"].get<std::string>();
        if (line_count == 0) assert(ev == "boot_start");
        if (line_count == 1) assert(ev == "boot_registry_loaded");
        if (line_count == 2) assert(ev == "execution_start");
        if (line_count == 3) {
            assert(ev == "execution_success");
            assert(j.contains("duration_ms"));
        }
        if (line_count == 4) assert(ev == "status_snapshot");
        
        line_count++;
    }
    
    assert(line_count == 5);
    ifs.close();

    std::filesystem::remove(audit_file);
    std::cout << "Operational Harness: All checks passed!\n";
    return 0;
}
