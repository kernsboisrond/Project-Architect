#pragma once
#include "core/AgentContext.hpp"
#include <string>

namespace Architect::Warden {

class PromptAssembler {
public:
    [[nodiscard]]
    static std::string BuildPrompt(const Architect::Core::AgentContext& context);
};

} // namespace Architect::Warden
