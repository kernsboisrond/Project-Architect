#pragma once

#include "core/AgentContext.hpp"
#include "seraph/IExecutor.hpp"
#include "seraph/IAuditSink.hpp"
#include "warden/CognitiveFrame.hpp"
#include "warden/WardenEngine.hpp"
#include <string>
#include <vector>

struct DispatchOutcome {
    bool success{true};
    std::string error_message{};
};

class CognitiveLoop {
private:
    Architect::Warden::Engine& engine_;
    Architect::Seraph::IExecutor& executor_;
    Architect::Seraph::IAuditSink& audit_;
    const Architect::Seraph::CapabilityManifest system_capabilities_;
    std::vector<std::string> prompt_capabilities_;

public:
    CognitiveLoop(Architect::Warden::Engine& engine,
                  Architect::Seraph::IExecutor& executor,
                  Architect::Seraph::IAuditSink& audit,
                  const Architect::Seraph::CapabilityManifest& system_capabilities,
                  std::vector<std::string> prompt_capabilities);

    void run();

private:
    DispatchOutcome DispatchIntent(const Architect::Warden::CognitiveFrame& frame) const;
};
