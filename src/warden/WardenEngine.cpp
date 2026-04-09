#include "WardenEngine.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using json = nlohmann::json;

namespace Architect::Warden {
namespace {

bool HasOnlyKeys(
    const json& object,
    std::initializer_list<std::string_view> allowed_keys
) {
    if (!object.is_object()) {
        return false;
    }

    for (const auto& [key, _] : object.items()) {
        bool allowed = false;
        for (const auto allowed_key : allowed_keys) {
            if (key == allowed_key) {
                allowed = true;
                break;
            }
        }

        if (!allowed) {
            return false;
        }
    }

    return true;
}

bool TryReadUInt64(const json& value, std::uint64_t& out) {
    if (value.is_number_unsigned()) {
        out = value.get<std::uint64_t>();
        return true;
    }

    if (value.is_number_integer()) {
        const auto signed_value = value.get<std::int64_t>();
        if (signed_value < 0) {
            return false;
        }

        out = static_cast<std::uint64_t>(signed_value);
        return true;
    }

    return false;
}

bool TryReadByte(const json& value, std::uint8_t& out) {
    std::uint64_t temp = 0;
    if (!TryReadUInt64(value, temp)) {
        return false;
    }

    if (temp > std::numeric_limits<std::uint8_t>::max()) {
        return false;
    }

    out = static_cast<std::uint8_t>(temp);
    return true;
}

bool TryReadStringMap(
    const json& value,
    std::unordered_map<std::string, std::string>& out
) {
    if (!value.is_object()) {
        return false;
    }

    out.clear();

    for (const auto& [key, mapped] : value.items()) {
        if (!mapped.is_string()) {
            return false;
        }
        out.emplace(key, mapped.get<std::string>());
    }

    return true;
}

} // namespace

void Engine::CompileGrammarConstraints() {
    if (!active_gbnf_grammar_.empty()) {
        return;
    }

    std::cout << "Engine: Compiling strict GBNF grammar constraints for local LLM inference...\n";

    active_gbnf_grammar_ = R"(root ::= "{" ws "\"frame_id\":" ws number "," ws "\"timestamp_ms\":" ws number "," ws "\"intent_type\":" ws intent_type "," ws "\"payload\":" ws payload "}"
ws ::= [ \t\n\r]*
string ::= "\"" chars "\""
chars ::= "" | char chars
char ::= [^"\\] | "\\" escape
escape ::= "\"" | "\\" | "/" | "b" | "f" | "n" | "r" | "t"
number ::= [0-9]+
number_array ::= "[" ws (number (ws "," ws number)*)? ws "]"
dict ::= "{" ws (string ws ":" ws string (ws "," ws string ws ":" ws string)*)? ws "}"

intent_type ::= "\"System2Think\"" | "\"QueryMerovingian\"" | "\"InvokeSeraph\"" | "\"BroadcastSmith\""
payload ::= payload_think | payload_query | payload_invoke | payload_broadcast

payload_think ::= "{" ws "\"internal_monologue\":" ws string ws "}"
payload_query ::= "{" ws "\"entity_node_id\":" ws string ws "," ws "\"relation_type\":" ws string ws "}"
payload_invoke ::= "{" ws "\"target_wasm_module\":" ws string ws "," ws "\"target_function\":" ws string ws "," ws "\"arguments\":" ws dict ws "}"
payload_broadcast ::= "{" ws "\"target_agent_id\":" ws number ws "," ws "\"binary_payload\":" ws number_array ws "}"
)";

    std::cout << "-> Grammar loaded. Model emissions will be forcefully restricted.\n";
}

std::expected<CognitiveFrame, WardenError>
Engine::EnforceCognition(std::string_view stimulus) {
    if (active_gbnf_grammar_.empty()) {
        CompileGrammarConstraints();
    }

    if (stimulus.empty()) {
        std::cerr << "Engine::EnforceCognition - GrammarViolation: empty stimulus.\n";
        return std::unexpected(WardenError::GrammarViolation);
    }

    // Phase 1.5 bridge:
    // For now, stimulus is still assumed to be the already-generated JSON frame.
    // Next step is to replace this with:
    // prompt -> backend generation -> DeserializeSIMD(raw_model_output)
    return DeserializeSIMD(stimulus);
}

std::expected<CognitiveFrame, WardenError>
Engine::DeserializeSIMD(std::string_view validated_json) const {
    try {
        const json j = json::parse(validated_json);

        if (!j.is_object()) {
            std::cerr << "Engine::DeserializeSIMD - GrammarViolation: root must be a JSON object.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        if (!HasOnlyKeys(j, {"frame_id", "timestamp_ms", "intent_type", "payload"})) {
            std::cerr << "Engine::DeserializeSIMD - GrammarViolation: unauthorized root field detected.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        if (!j.contains("frame_id") ||
            !j.contains("timestamp_ms") ||
            !j.contains("intent_type") ||
            !j.contains("payload")) {
            std::cerr << "Engine::DeserializeSIMD - GrammarViolation: missing required root field(s).\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        std::uint64_t frame_id = 0;
        std::uint64_t timestamp_ms = 0;

        if (!TryReadUInt64(j.at("frame_id"), frame_id)) {
            std::cerr << "Engine::DeserializeSIMD - GrammarViolation: frame_id must be a non-negative integer.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        if (!TryReadUInt64(j.at("timestamp_ms"), timestamp_ms)) {
            std::cerr << "Engine::DeserializeSIMD - GrammarViolation: timestamp_ms must be a non-negative integer.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        if (!j.at("intent_type").is_string()) {
            std::cerr << "Engine::DeserializeSIMD - GrammarViolation: intent_type must be a string.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        const std::string intent_type = j.at("intent_type").get<std::string>();
        const json& payload = j.at("payload");

        if (!payload.is_object()) {
            std::cerr << "Engine::DeserializeSIMD - GrammarViolation: payload must be an object.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        CognitiveFrame frame{};
        frame.frame_id = frame_id;
        frame.timestamp_ms = timestamp_ms;

        if (intent_type == "System2Think") {
            if (!HasOnlyKeys(payload, {"internal_monologue"}) ||
                !payload.contains("internal_monologue") ||
                !payload.at("internal_monologue").is_string()) {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: invalid System2Think payload.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            frame.intent = System2Think{
                payload.at("internal_monologue").get<std::string>()
            };
            return frame;
        }

        if (intent_type == "QueryMerovingian") {
            if (!HasOnlyKeys(payload, {"entity_node_id", "relation_type"}) ||
                !payload.contains("entity_node_id") ||
                !payload.contains("relation_type") ||
                !payload.at("entity_node_id").is_string() ||
                !payload.at("relation_type").is_string()) {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: invalid QueryMerovingian payload.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            frame.intent = QueryMerovingian{
                payload.at("entity_node_id").get<std::string>(),
                payload.at("relation_type").get<std::string>()
            };
            return frame;
        }

        if (intent_type == "InvokeSeraph") {
            if (!HasOnlyKeys(payload, {"target_wasm_module", "target_function", "arguments"}) ||
                !payload.contains("target_wasm_module") ||
                !payload.contains("target_function") ||
                !payload.contains("arguments") ||
                !payload.at("target_wasm_module").is_string() ||
                !payload.at("target_function").is_string()) {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: invalid InvokeSeraph payload.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            std::unordered_map<std::string, std::string> args;
            if (!TryReadStringMap(payload.at("arguments"), args)) {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: InvokeSeraph arguments must be a string:string map.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            frame.intent = InvokeSeraph{
                payload.at("target_wasm_module").get<std::string>(),
                payload.at("target_function").get<std::string>(),
                std::move(args)
            };
            return frame;
        }

        if (intent_type == "BroadcastSmith") {
            if (!HasOnlyKeys(payload, {"target_agent_id", "binary_payload"}) ||
                !payload.contains("target_agent_id") ||
                !payload.contains("binary_payload") ||
                !payload.at("binary_payload").is_array()) {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: invalid BroadcastSmith payload.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            std::uint64_t target_agent_id = 0;
            if (!TryReadUInt64(payload.at("target_agent_id"), target_agent_id)) {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: target_agent_id must be a non-negative integer.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            std::vector<std::uint8_t> binary_payload;
            binary_payload.reserve(payload.at("binary_payload").size());

            for (const auto& byte_val : payload.at("binary_payload")) {
                std::uint8_t parsed_byte = 0;
                if (!TryReadByte(byte_val, parsed_byte)) {
                    std::cerr << "Engine::DeserializeSIMD - GrammarViolation: binary_payload must contain bytes in range 0..255.\n";
                    return std::unexpected(WardenError::GrammarViolation);
                }
                binary_payload.push_back(parsed_byte);
            }

            frame.intent = BroadcastSmith{
                target_agent_id,
                std::move(binary_payload)
            };
            return frame;
        }

        std::cerr << "Engine::DeserializeSIMD - UnauthorizedIntent: unknown intent_type: "
                  << intent_type << "\n";
        return std::unexpected(WardenError::UnauthorizedIntent);

    } catch (const json::parse_error& e) {
        std::cerr << "Engine::DeserializeSIMD - JSON parser error: " << e.what() << "\n";
        return std::unexpected(WardenError::GrammarViolation);
    } catch (const json::type_error& e) {
        std::cerr << "Engine::DeserializeSIMD - JSON type error: " << e.what() << "\n";
        return std::unexpected(WardenError::GrammarViolation);
    } catch (const json::out_of_range& e) {
        std::cerr << "Engine::DeserializeSIMD - JSON field missing: " << e.what() << "\n";
        return std::unexpected(WardenError::GrammarViolation);
    }
}

} // namespace Architect::Warden
