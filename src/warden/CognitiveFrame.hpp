#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Architect::Warden {

struct System2Think {
    std::string internal_monologue;
};

struct QueryMerovingian {
    std::string entity_node_id;
    std::string relation_type;
};

struct InvokeSeraph {
    std::string target_wasm_module;
    std::unordered_map<std::string, std::string> arguments;
};

struct BroadcastSmith {
    uint64_t target_agent_id;
    std::vector<uint8_t> binary_payload;
};

using AgentIntent = std::variant<
    System2Think,
    QueryMerovingian,
    InvokeSeraph,
    BroadcastSmith
>;

struct CognitiveFrame {
    uint64_t frame_id;
    uint64_t timestamp_ms;
    AgentIntent intent;
};

enum class WardenError {
    GrammarViolation,
    InferenceTimeout,
    UnauthorizedIntent
};

} // namespace Architect::Warden
