#include "core/CognitiveLoop.hpp"
#include "seraph/ExecutorStub.hpp"
#include "warden/IBrainBackend.hpp"
#include "warden/LlamaCppBackend.hpp"
#include "warden/MockBrainBackend.hpp"
#include "warden/WardenEngine.hpp"

#include <iostream>
#include <memory>

int main() {
    std::cout << "Project ARCHITECT Local Kernel initiating...\n";

    std::unique_ptr<Architect::Warden::IBrainBackend> brain;

#if ARCHITECT_ENABLE_LLAMA
    Architect::Warden::LlamaBackendConfig config;
    config.model_path = ""; // Fill this later during real Gemma bring-up.

    if (!config.model_path.empty()) {
        brain = std::make_unique<Architect::Warden::LlamaCppBackend>(config);
        std::cout << "[Boot] LlamaCppBackend selected.\n";
    } else {
        brain = std::make_unique<Architect::Warden::MockBrainBackend>();
        std::cout << "[Boot] MockBrainBackend selected (no model path configured).\n";
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
