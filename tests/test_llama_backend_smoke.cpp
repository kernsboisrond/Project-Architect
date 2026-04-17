#include "warden/LlamaCppBackend.hpp"
#include "warden/GrammarConstraints.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>

int main() {
    using namespace Architect::Warden;

#if ARCHITECT_ENABLE_LLAMA
    LlamaBackendConfig config;
    
    // Allow overriding from environment if the user wishes, default to gemma-4-e2b.gguf
    if (const char* env_path = std::getenv("ARCHITECT_MODEL_PATH")) {
        config.model_path = env_path;
    } else {
        config.model_path = "models/gemma-4-e2b.gguf";
    }
    
    config.n_ctx = 1024;
    config.n_predict = 256; // Increased predict limit to allow seeing full JSON structures

    if (!std::filesystem::exists(config.model_path)) {
        std::cout << "[Test] Model file not found at: " << config.model_path << "\n";
        std::cout << "[Test] Skipping llama backend smoke test.\n";
        return 0;
    }

    std::cout << "[Test] Constructing LlamaCppBackend Phase 4A/4B Smoke Probe...\n";
    LlamaCppBackend backend{config};

    if (!backend.IsReady()) {
        std::cerr << "[Test] FAILED: Backend not ready after construction.\n";
        return 1;
    }
    std::cout << "[Test] Backend is ready.\n\n";

    std::cout << "--- CONSTRAINED GENERATION: INVOKESERAPH ---\n";
    std::string prompt1 = "Print hello through echo.";
    std::cout << "[Prompt]: " << prompt1 << "\n";
    auto result_1 = backend.Generate(prompt1, Architect::Warden::GetCognitiveFrameGrammar());
    if (result_1) std::cout << "[Raw Output]:\n" << *result_1 << "\n----------------------------\n\n";

    std::cout << "--- CONSTRAINED GENERATION: SYSTEM2THINK ---\n";
    std::string prompt2 = "Think privately about a safe hello.";
    std::cout << "[Prompt]: " << prompt2 << "\n";
    auto result_2 = backend.Generate(prompt2, Architect::Warden::GetCognitiveFrameGrammar());
    if (result_2) std::cout << "[Raw Output]:\n" << *result_2 << "\n----------------------------\n\n";

    std::cout << "--- CONSTRAINED GENERATION: QUERYMEROVINGIAN ---\n";
    std::string prompt3 = "Query the relation for entity hello.";
    std::cout << "[Prompt]: " << prompt3 << "\n";
    auto result_3 = backend.Generate(prompt3, Architect::Warden::GetCognitiveFrameGrammar());
    if (result_3) std::cout << "[Raw Output]:\n" << *result_3 << "\n----------------------------\n\n";

    std::cout << "[Test] Smoke test completed successfully.\n";
    return 0;
#else
    std::cout << "[Test] ARCHITECT_ENABLE_LLAMA is OFF. Skipping test.\n";
    return 0;
#endif
}
