#include "warden/LlamaCppBackend.hpp"
#include "warden/WardenEngine.hpp"
#include "core/AgentContext.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include <variant>
#include <cstdint>

#if ARCHITECT_ENABLE_LLAMA
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

bool RunIntentTest(Architect::Warden::Engine& engine, 
                   const std::string& stimulus, 
                   const std::string& expected_intent_name,
                   std::uint64_t& last_frame_id) 
{
    std::cout << "--- ENFORCE COGNITION TEST ---\n";
    std::cout << "[Stimulus]: " << stimulus << "\n";
    std::cout << "[Expected]: " << expected_intent_name << "\n";

    Architect::Core::AgentContext ctx;
    ctx.current_stimulus = stimulus;
    ctx.available_capabilities = {"echo::print"};
    
    auto result = engine.EnforceCognition(ctx);

    if (!result.has_value()) {
        std::cerr << "[Result]: FAILED - Warden rejected the output format entirely.\n\n";
        return false;
    }
    
    if (result->frame_id == 0) {
        std::cerr << "[Result]: FAILED - frame_id is 0.\n";
        return false;
    }
    if (result->frame_id <= last_frame_id) {
        std::cerr << "[Result]: FAILED - frame_id did not strictly increase. Last: " << last_frame_id << " New: " << result->frame_id << "\n";
        return false;
    }
    if (result->timestamp_ms == 0) {
        std::cerr << "[Result]: FAILED - timestamp_ms is 0.\n";
        return false;
    }
    
    last_frame_id = result->frame_id;
    
    bool matched = false;

    if (expected_intent_name == "System2Think") {
        if (std::holds_alternative<Architect::Warden::System2Think>(result->intent)) {
            matched = true;
        }
    } else if (expected_intent_name == "QueryMerovingian") {
        if (std::holds_alternative<Architect::Warden::QueryMerovingian>(result->intent)) {
            matched = true;
        }
    } else if (expected_intent_name == "InvokeSeraph") {
        if (auto* invoke = std::get_if<Architect::Warden::InvokeSeraph>(&result->intent)) {
            if (invoke->target_wasm_module == "echo" && invoke->target_function == "print") {
                if (invoke->arguments.contains("message")) {
                    matched = true;
                } else {
                    std::cerr << "[Result]: FAILED - InvokeSeraph missing 'message' argument.\n";
                }
            } else {
                std::cerr << "[Result]: FAILED - Expected echo::print, got " << invoke->target_wasm_module << "::" << invoke->target_function << "\n";
            }
        }
    } else if (expected_intent_name == "BroadcastSmith") {
        if (std::holds_alternative<Architect::Warden::BroadcastSmith>(result->intent)) {
            matched = true;
        }
    }

    if (matched) {
        std::cout << "[Result]: PASS\n\n";
        return true;
    } else {
        std::cerr << "[Result]: FAILED - Expected " << expected_intent_name << " intent mismatch.\n\n";
        return false;
    }
}

} // namespace
#endif

int main() {
    using namespace Architect::Warden;

#if ARCHITECT_ENABLE_LLAMA
    LlamaBackendConfig config;
    config.model_path = ReadEnvString("ARCHITECT_MODEL_PATH", "models/gemma-4-e2b.gguf");
    config.n_ctx      = ReadEnvInt("ARCHITECT_N_CTX", 2048);
    config.n_predict  = ReadEnvInt("ARCHITECT_N_PREDICT", 128);
    config.n_batch    = ReadEnvInt("ARCHITECT_N_BATCH", 512);
    config.n_ubatch   = ReadEnvInt("ARCHITECT_N_UBATCH", 256);

    if (!std::filesystem::exists(config.model_path)) {
        std::cout << "[Test] Model file not found at: " << config.model_path << "\n";
        std::cout << "[Test] Skipping real backend intent tests.\n";
        return 0;
    }

    std::cout << "[Test] Constructing Warden Engine with LlamaCppBackend phase 7 validation...\n";
    auto backend = std::make_unique<LlamaCppBackend>(config);

    if (!backend->IsReady()) {
        std::cerr << "[Test] FAILED: Backend not ready after construction.\n";
        return 1;
    }

    Engine engine{std::move(backend)};
    std::cout << "[Test] Engine and Backend are ready.\n\n";

    bool all_passed = true;
    std::uint64_t last_frame_id = 0;

    all_passed &= RunIntentTest(engine, "Think privately about a safe hello.", "System2Think", last_frame_id);
    all_passed &= RunIntentTest(engine, "Query the relation for entity hello.", "QueryMerovingian", last_frame_id);
    all_passed &= RunIntentTest(engine, "Say hello using echo.", "InvokeSeraph", last_frame_id);
    all_passed &= RunIntentTest(engine, "Print hello through echo.", "InvokeSeraph", last_frame_id);

    if (all_passed) {
        std::cout << "[Test] ALL TESTS PASSED. Warden acceptance validation completed successfully.\n";
        return 0;
    } else {
        std::cerr << "[Test] ONE OR MORE TESTS FAILED.\n";
        return 1;
    }
#else
    std::cout << "[Test] ARCHITECT_ENABLE_LLAMA is OFF. Skipping test.\n";
    return 0;
#endif
}
