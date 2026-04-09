#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Architect::Warden {

struct System2Think {
    std::string internal_monologue{};
};

struct QueryMerovingian {
    std::string entity_node_id{};
    std::string relation_type{};
};

struct InvokeSeraph {
    std::string target_wasm_module{};
    std::string target_function{};
    std::unordered_map<std::string, std::string> arguments{};
};

struct BroadcastSmith {
    std::uint64_t target_agent_id{0};
    std::vector<std::uint8_t> binary_payload{};
};

using AgentIntent = std::variant<
    System2Think,
    QueryMerovingian,
    InvokeSeraph,
    BroadcastSmith
>;

struct CognitiveFrame {
    std::uint64_t frame_id{0};
    std::uint64_t timestamp_ms{0};
    AgentIntent intent;
};

enum class WardenError {
    GrammarViolation,
    InferenceTimeout,
    UnauthorizedIntent
};

} // namespace Architect::Warden
