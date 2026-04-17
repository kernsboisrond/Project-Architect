#include "core/CognitiveLoop.hpp"
#include "seraph/ExecutorStub.hpp"
#include "warden/IBrainBackend.hpp"
#include "warden/LlamaCppBackend.hpp"
#include "warden/MockBrainBackend.hpp"
#include "warden/WardenEngine.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

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

int main() {
    std::cout << "Project ARCHITECT Local Kernel initiating...\n";

    std::unique_ptr<Architect::Warden::IBrainBackend> brain;

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
        std::cout << "[Boot] Resolved config:"
                  << " model_path=" << config.model_path
                  << ", n_ctx=" << config.n_ctx
                  << ", n_predict=" << config.n_predict
                  << ", n_batch=" << config.n_batch
                  << ", n_ubatch=" << config.n_ubatch
                  << ", n_gpu_layers=" << config.n_gpu_layers
                  << ", verbose=" << (config.verbose ? "true" : "false")
                  << "\n";

        brain = std::make_unique<Architect::Warden::LlamaCppBackend>(config);

        if (auto* llama = dynamic_cast<Architect::Warden::LlamaCppBackend*>(brain.get());
            llama && !llama->IsReady()) {
            std::cout << "[Boot] LlamaCppBackend failed to initialize. Falling back to MockBrainBackend.\n";
            brain = std::make_unique<Architect::Warden::MockBrainBackend>();
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
    Architect::Seraph::ExecutorStub executor{};

    CognitiveLoop heartbeat{engine, executor};
    heartbeat.run();

    std::cout << "Kernel shutting down...\n";
    return 0;
}
