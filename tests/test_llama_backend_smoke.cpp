#include "warden/LlamaCppBackend.hpp"
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
    config.n_predict = 32; // Small predict limit to keep the test short

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

    std::vector<std::string> prompts = {
        "Say exactly HELLO and nothing else.",
        "Return a short JSON object with one key named test.",
        "Say exactly HELLO and nothing else." // testing potential cross-call contamination (Phase 4B)
    };

    int pass = 1;
    for (const auto& prompt : prompts) {
        std::cout << "--- GENERATION PASS " << pass++ << " ---\n";
        std::cout << "[Prompt]: " << prompt << "\n";
        
        auto result = backend.Generate(prompt, "");
        if (!result) {
            std::cerr << "[Test] FAILED: Generation failed on pass " << (pass-1) << ".\n";
            return 1;
        }

        std::cout << "[Raw Output]:\n" << *result << "\n----------------------------\n\n";
    }

    std::cout << "[Test] Smoke test completed successfully.\n";
    return 0;
#else
    std::cout << "[Test] ARCHITECT_ENABLE_LLAMA is OFF. Skipping test.\n";
    return 0;
#endif
}
