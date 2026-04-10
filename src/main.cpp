#include "core/CognitiveLoop.hpp"
#include "seraph/ExecutorStub.hpp"
#include "warden/MockBrainBackend.hpp"
#include "warden/WardenEngine.hpp"

#include <iostream>
#include <memory>

int main() {
    std::cout << "Project ARCHITECT Local Kernel initiating...\n";

    auto brain = std::make_unique<Architect::Warden::MockBrainBackend>();
    Architect::Warden::Engine engine{std::move(brain)};
    Architect::Seraph::ExecutorStub executor{};

    CognitiveLoop heartbeat{engine, executor};
    heartbeat.run();

    std::cout << "Kernel shutting down...\n";
    return 0;
}
