#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace Architect::Core {

struct AgentContext {
    std::string current_stimulus{};
    std::vector<std::string> available_capabilities{};
    std::vector<std::string> memory_pointers{};
    std::optional<std::string> last_action_feedback{};
    std::size_t recovery_attempts{0};
};

} // namespace Architect::Core
