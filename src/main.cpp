#include "core/CognitiveLoop.hpp"
#include "warden/WardenEngine.hpp"
#include "seraph/ExecutorStub.hpp"

#include <iostream>

int main() {
    std::cout << "Project ARCHITECT Local Kernel initiating...\n";

    Architect::Warden::Engine engine{};
    Architect::Seraph::ExecutorStub executor{};
    CognitiveLoop heartbeat{engine, executor};

    heartbeat.run();

    std::cout << "Kernel shutting down...\n";
    return 0;
}
