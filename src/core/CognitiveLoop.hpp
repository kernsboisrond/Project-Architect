#pragma once

#include "core/AgentContext.hpp"
#include "seraph/ExecutorStub.hpp"
#include "warden/CognitiveFrame.hpp"
#include "warden/WardenEngine.hpp"

#include <string>

struct DispatchOutcome {
    bool success{true};
    std::string error_message{};
};

class CognitiveLoop {
private:
    Architect::Warden::Engine& engine_;
    Architect::Seraph::ExecutorStub& executor_;

public:
    CognitiveLoop(Architect::Warden::Engine& engine,
                  Architect::Seraph::ExecutorStub& executor);

    void run();

private:
    DispatchOutcome DispatchIntent(const Architect::Warden::CognitiveFrame& frame) const;
};
