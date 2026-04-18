#include "warden/LlamaCppBackend.hpp"
#include "warden/GrammarConstraints.hpp"
#include "warden/PromptAssembler.hpp"
#include "core/AgentContext.hpp"

#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>

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

bool RunIntentTest(Architect::Warden::LlamaCppBackend& backend, 
                   const std::string& stimulus, 
                   const std::string& expected_intent, 
                   const std::vector<std::string>& expected_payload_keys) 
{
    using json = nlohmann::json;

    std::cout << "--- CONSTRAINED GENERATION TEST ---\n";
    std::cout << "[Stimulus]: " << stimulus << "\n";
    std::cout << "[Expected]: " << expected_intent << "\n";

    Architect::Core::AgentContext ctx;
    ctx.current_stimulus = stimulus;
    ctx.available_capabilities = {"echo::print"};
    
    std::string prompt = Architect::Warden::PromptAssembler::BuildPrompt(ctx);
    
    auto result = backend.Generate(prompt, Architect::Warden::GetCognitiveFrameGrammar());
    if (!result.has_value()) {
        std::cerr << "[Result]: FAILED - Backend generation failed.\n\n";
        return false;
    }
    
    try {
        json j = json::parse(*result);
        
        if (j.contains("frame_id") || j.contains("timestamp_ms")) {
            std::cerr << "[Result]: FAILED - Model output illegally contains frame_id or timestamp_ms.\n";
            std::cerr << "[Raw Output]:\n" << *result << "\n\n";
            return false;
        }

        if (!j.contains("intent_type") || !j.at("intent_type").is_string()) {
            std::cerr << "[Result]: FAILED - Missing or invalid 'intent_type'.\n";
            std::cerr << "[Raw Output]:\n" << *result << "\n\n";
            return false;
        }

        std::string actual_intent = j.at("intent_type").get<std::string>();
        if (actual_intent != expected_intent) {
            std::cerr << "[Result]: FAILED - Expected " << expected_intent << " but got " << actual_intent << "\n";
            std::cerr << "[Raw Output]:\n" << *result << "\n\n";
            return false;
        }
        
        if (!j.contains("payload") || !j.at("payload").is_object()) {
            std::cerr << "[Result]: FAILED - Missing or invalid 'payload' object.\n";
            std::cerr << "[Raw Output]:\n" << *result << "\n\n";
            return false;
        }

        const auto& payload = j.at("payload");
        if (payload.size() != expected_payload_keys.size()) {
            std::cerr << "[Result]: FAILED - Payload key count mismatch. Expected " << expected_payload_keys.size() << " keys but got " << payload.size() << ".\n";
            std::cerr << "[Raw Output]:\n" << *result << "\n\n";
            return false;
        }

        for (const auto& key : expected_payload_keys) {
            if (!payload.contains(key)) {
                std::cerr << "[Result]: FAILED - Expected payload key '" << key << "' missing.\n";
                std::cerr << "[Raw Output]:\n" << *result << "\n\n";
                return false;
            }
        }
        
        // Context-specific validation for InvokeSeraph echo::print
        if (expected_intent == "InvokeSeraph") {
             std::string mod = payload.value("target_wasm_module", "");
             std::string func = payload.value("target_function", "");
             if (mod != "echo" || func != "print") {
                 std::cerr << "[Result]: FAILED - Expected echo::print but got " << mod << "::" << func << "\n";
                 std::cerr << "[Raw Output]:\n" << *result << "\n\n";
                 return false;
             }
             if (!payload.contains("arguments") || !payload.at("arguments").contains("message")) {
                 std::cerr << "[Result]: FAILED - Expected arguments.message in InvokeSeraph.\n";
                 std::cerr << "[Raw Output]:\n" << *result << "\n\n";
                 return false;
             }
        }
        
        std::cout << "[Result]: PASS\n\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Result]: FAILED - JSON parsing/validation error: " << e.what() << "\n";
        std::cerr << "[Raw Output]:\n" << *result << "\n\n";
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
    config.n_ctx = ReadEnvInt("ARCHITECT_N_CTX", 2048);
    config.n_predict = ReadEnvInt("ARCHITECT_N_PREDICT", 128);
    config.n_batch = ReadEnvInt("ARCHITECT_N_BATCH", 512);
    config.n_ubatch = ReadEnvInt("ARCHITECT_N_UBATCH", 256);

    if (!std::filesystem::exists(config.model_path)) {
        std::cout << "[Test] Model file not found at: " << config.model_path << "\n";
        std::cout << "[Test] Skipping llama backend smoke test.\n";
        return 0;
    }

    std::cout << "[Test] Constructing LlamaCppBackend Phase 7 Smoke Probe...\n";
    LlamaCppBackend backend{config};

    if (!backend.IsReady()) {
        std::cerr << "[Test] FAILED: Backend not ready after construction.\n";
        return 1;
    }
    std::cout << "[Test] Backend is ready.\n\n";

    bool all_passed = true;

    all_passed &= RunIntentTest(backend, "Think privately about a safe hello.", "System2Think", {"internal_monologue"});
    all_passed &= RunIntentTest(backend, "Query the relation for entity hello.", "QueryMerovingian", {"entity_node_id", "relation_type"});
    all_passed &= RunIntentTest(backend, "Say hello using echo.", "InvokeSeraph", {"target_wasm_module", "target_function", "arguments"});
    all_passed &= RunIntentTest(backend, "Print hello through echo.", "InvokeSeraph", {"target_wasm_module", "target_function", "arguments"});

    if (all_passed) {
        std::cout << "[Test] ALL TESTS PASSED. Smoke test completed successfully.\n";
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
