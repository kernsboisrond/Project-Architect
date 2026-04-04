#pragma once
#include "warden/CognitiveFrame.hpp"
#include "warden/WardenEngine.hpp"

class CognitiveLoop {
private:
    Architect::Warden::Engine& engine_;

public:
    explicit CognitiveLoop(Architect::Warden::Engine& engine);
    void run();
    
    // Core heartbeat dispatcher mechanism
    void DispatchIntent(const Architect::Warden::CognitiveFrame& frame);
};
