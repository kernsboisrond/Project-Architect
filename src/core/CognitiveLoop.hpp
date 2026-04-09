#pragma once

#include "warden/CognitiveFrame.hpp"
#include "warden/WardenEngine.hpp"
#include "seraph/ExecutorStub.hpp"

class CognitiveLoop {
private:
    Architect::Warden::Engine& engine_;
    Architect::Seraph::ExecutorStub& executor_;

public:
    explicit CognitiveLoop(Architect::Warden::Engine& engine, Architect::Seraph::ExecutorStub& executor);

    void run();

private:
    // Core heartbeat dispatcher mechanism
    void DispatchIntent(const Architect::Warden::CognitiveFrame& frame) const;
};
