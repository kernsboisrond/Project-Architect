#include "core/CognitiveLoop.hpp"
#include "seraph/ExecutorStub.hpp"
#include "seraph/FileAuditSink.hpp"
#include "seraph/WasmExecutor.hpp"
#include "seraph/ModuleRegistry.hpp"
#include "seraph/InvocationTypes.hpp"
#include "warden/IBrainBackend.hpp"
#include "warden/LlamaCppBackend.hpp"
#include "warden/MockBrainBackend.hpp"
#include "warden/WardenEngine.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace {

std::string ReadEnvString(const char* name, std::string fallback) {
    if (const char* value = std::getenv(name)) {
        return std::string(value);
    }
    return fallback;
}

int ReadEnvInt(const char* name, int fallback) {
    if (const char* value = std::getenv(name)) {
        try {
            return std::stoi(value);
        } catch (...) {
            return fallback;
        }
    }
    return fallback;
}

bool ReadEnvBool(const char* name, bool fallback) {
    if (const char* value = std::getenv(name)) {
        const std::string_view v{value};
        if (v == "1" || v == "true" || v == "TRUE" || v == "on" || v == "ON") {
            return true;
        }
        if (v == "0" || v == "false" || v == "FALSE" || v == "off" || v == "OFF") {
            return false;
        }
    }
    return fallback;
}

} // namespace

int main(int argc, char** argv) {
    bool status_mode = ReadEnvBool("ARCHITECT_STATUS", false);
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--status") {
            status_mode = true;
        }
    }

    std::string audit_path = ReadEnvString("ARCHITECT_AUDIT_LOG", "architect_audit.jsonl");
    Architect::Seraph::FileAuditSink audit{audit_path};
    audit.LogSystemEvent("boot_start");

    std::cout << "Project ARCHITECT Local Kernel initiating...\n";

    std::unique_ptr<Architect::Warden::IBrainBackend> brain;
    std::string brain_kind = "mock";
    std::string last_error = "none";

#if ARCHITECT_ENABLE_LLAMA
    Architect::Warden::LlamaBackendConfig config;
    config.model_path   = ReadEnvString("ARCHITECT_MODEL_PATH", "models/gemma-4-e2b.gguf");
    config.n_ctx        = ReadEnvInt("ARCHITECT_N_CTX", 1024);
    config.n_predict    = ReadEnvInt("ARCHITECT_N_PREDICT", 128);
    config.n_batch      = ReadEnvInt("ARCHITECT_N_BATCH", 512);
    config.n_ubatch     = ReadEnvInt("ARCHITECT_N_UBATCH", 256);
    config.n_gpu_layers = ReadEnvInt("ARCHITECT_N_GPU_LAYERS", 0);
    config.verbose      = ReadEnvBool("ARCHITECT_VERBOSE", false);

    if (!config.model_path.empty()) {
        std::cout << "[Boot] LlamaCppBackend selected.\n";
        brain_kind = "llama_cpp";
        brain = std::make_unique<Architect::Warden::LlamaCppBackend>(config);

        if (auto* llama = dynamic_cast<Architect::Warden::LlamaCppBackend*>(brain.get());
            llama && !llama->IsReady()) {
            std::cout << "[Boot] LlamaCppBackend failed to initialize. Falling back to MockBrainBackend.\n";
            last_error = "llama_cpp init failed";
            brain = std::make_unique<Architect::Warden::MockBrainBackend>();
            brain_kind = "mock_fallback";
        }
    } else {
        brain = std::make_unique<Architect::Warden::MockBrainBackend>();
        std::cout << "[Boot] MockBrainBackend selected (empty model path).\n";
    }
#else
    brain = std::make_unique<Architect::Warden::MockBrainBackend>();
    std::cout << "[Boot] MockBrainBackend selected.\n";
#endif

    Architect::Warden::Engine engine{std::move(brain)};
    
    std::unique_ptr<Architect::Seraph::IExecutor> executor;
    Architect::Seraph::CapabilityManifest system_policy;
    std::vector<std::string> prompt_capabilities;
    
    std::string executor_type = "stub";
    std::string active_manifest = "none";
    size_t cached_modules = 0;
    Architect::Seraph::RegistryDiagnostics diag{};

    const char* executor_env = std::getenv("ARCHITECT_EXECUTOR");
    if (executor_env && std::string(executor_env) == "wasm") {
        executor_type = "wasm";
        std::string manifest_path = ReadEnvString("ARCHITECT_WASM_MANIFEST", "./tests/fixtures/manifest.json");
        active_manifest = manifest_path;
        
        nlohmann::json exec_j = {{"executor", "wasm"}, {"manifest", manifest_path}};
        audit.LogSystemEvent("boot_executor_selected", exec_j.dump());
        std::cout << "[Boot] Attempting to mount WasmExecutor using manifest " << manifest_path << "\n";
        
        try {
            auto registry = std::make_shared<Architect::Seraph::ModuleRegistry>();
            auto load_res = registry->LoadManifest(manifest_path);
            if (!load_res.has_value()) {
                 std::string err_msg;
                 switch (load_res.error()) {
                     case Architect::Seraph::RegistryError::ManifestNotFound: err_msg = "Manifest missing"; break;
                     case Architect::Seraph::RegistryError::ManifestParseFailed: err_msg = "Manifest parse failed"; break;
                     case Architect::Seraph::RegistryError::InvalidSchema: err_msg = "Invalid schema"; break;
                     case Architect::Seraph::RegistryError::UnsupportedManifestVersion: err_msg = "Unsupported manifest version"; break;
                     default: err_msg = "Unknown registry error";
                 }
                 last_error = "Registry fail: " + err_msg;
                 nlohmann::json err_j = {{"error", err_msg}};
                 audit.LogSystemEvent("boot_registry_failed", err_j.dump());
                 std::cerr << "[Boot] FATAL: Failed to load Wasm Registry manifest (" << err_msg << ").\n";
                 return 1;
            }
            
            auto wasm_exec = std::make_unique<Architect::Seraph::WasmExecutor>(registry);
            cached_modules = wasm_exec->InspectCacheCount();
            executor = std::move(wasm_exec);
            
            system_policy = registry->GenerateSystemPolicy();
            prompt_capabilities = registry->DescribePromptCapabilities();
            diag = registry->GetDiagnostics(cached_modules);
            
            nlohmann::json reg_j = {
                {"manifest", manifest_path},
                {"trusted", diag.trusted_modules},
                {"abi_compatible", diag.abi_compatible_modules},
                {"policy_exports", diag.policy_exports_count}
            };
            audit.LogSystemEvent("boot_registry_loaded", reg_j.dump());
            std::cout << "[Boot] WasmExecutor online.\n";
        } catch (...) {
            last_error = "WasmExecutor fatal exception";
            std::cerr << "[Boot] FATAL: WasmExecutor failed mapping unexpectedly.\n";
            return 1;
        }
    } else {
        std::cout << "[Boot] Defaulting to safe ExecutorStub. (set ARCHITECT_EXECUTOR=wasm to override)\n";
        executor = std::make_unique<Architect::Seraph::ExecutorStub>();
        system_policy.allowed_exports["echo"].push_back("print");
        prompt_capabilities = {"echo::print"};
        nlohmann::json exec_j = {{"executor", "stub"}};
        audit.LogSystemEvent("boot_executor_selected", exec_j.dump());
    }

    if (status_mode) {
        nlohmann::json status_j = {
            {"executor_type", executor_type},
            {"manifest_path", active_manifest},
            {"brain_backend", brain_kind},
            {"trusted_modules", diag.trusted_modules},
            {"cached_modules", cached_modules},
            {"abi_compatible_modules", diag.abi_compatible_modules},
            {"policy_exports_count", diag.policy_exports_count},
            {"last_error", last_error}
        };
        audit.LogSystemEvent("status_snapshot", status_j.dump());
        std::cout << "\n=== ARCHITECT STATUS ===\n";
        std::cout << status_j.dump(2) << "\n";
        std::cout << "========================\n";
        return 0; // Inspect, report, exit.
    }

    CognitiveLoop heartbeat{engine, *executor, audit, system_policy, prompt_capabilities};
    heartbeat.run();

    std::cout << "Kernel shutting down...\n";
    return 0;
}
