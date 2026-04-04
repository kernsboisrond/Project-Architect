#include "warden/WardenEngine.hpp"
#include "core/CognitiveLoop.hpp"
#include <iostream>

int main() {
    std::cout << "Project Architect Local Kernel initiating...\n";

    Architect::Warden::Engine engine;
    engine.CompileGrammarConstraints();
    
    CognitiveLoop heartbeat(engine);
    heartbeat.run();
    
    std::cout << "Kernel shutting down...\n";
    return 0;
}
