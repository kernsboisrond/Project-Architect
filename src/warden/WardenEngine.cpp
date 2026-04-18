#include "WardenEngine.hpp"

#include "GrammarConstraints.hpp"
#include "PromptAssembler.hpp"

#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using json = nlohmann::json;

namespace Architect::Warden {
namespace {

bool DebugRawOutputEnabled() {
    static const bool enabled = [] {
        if (const char* v = std::getenv("ARCHITECT_DEBUG_RAW_OUTPUT")) {
            return std::string_view(v) == "1";
        }
        return false;
    }();
    return enabled;
}

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
    active_gbnf_grammar_ = std::string(GetCognitiveFrameGrammar());
    std::cout << "-> Grammar loaded. Model emissions will be forcefully restricted.\n";
}

Engine::Engine(std::unique_ptr<IBrainBackend> brain)
    : brain_(std::move(brain)) {}

std::string_view Engine::ActiveGrammar() const noexcept {
    return active_gbnf_grammar_;
}

std::expected<CognitiveFrame, WardenError>
Engine::EnforceCognition(const Architect::Core::AgentContext& context) {
    if (active_gbnf_grammar_.empty()) {
        CompileGrammarConstraints();
    }

    if (!brain_) {
        std::cerr << "Engine::EnforceCognition - InferenceFailure: no brain backend attached.\n";
        return std::unexpected(WardenError::InferenceTimeout);
    }

    if (context.current_stimulus.empty() && !context.last_action_feedback.has_value()) {
        std::cerr << "Engine::EnforceCognition - GrammarViolation: empty stimulus and no recovery feedback.\n";
        return std::unexpected(WardenError::GrammarViolation);
    }

    const std::string prompt = PromptAssembler::BuildPrompt(context);

    auto raw_json = brain_->Generate(prompt, ActiveGrammar());
    if (!raw_json.has_value()) {
        std::cerr << "Engine::EnforceCognition - InferenceFailure: backend generation failed.\n";
        return std::unexpected(WardenError::InferenceTimeout);
    }

    if (raw_json->empty()) {
        std::cerr << "Engine::EnforceCognition - InferenceFailure: backend returned empty output.\n";
        return std::unexpected(WardenError::InferenceTimeout);
    }

    if (DebugRawOutputEnabled()) {
        std::cout << "[Warden] Raw backend output:\n" << *raw_json << "\n";
    }

    auto intent_result = DeserializeIntentOnly(*raw_json);
    if (!intent_result.has_value()) {
        return std::unexpected(intent_result.error());
    }

    CognitiveFrame frame{};
    frame.frame_id = NextFrameId();
    frame.timestamp_ms = CurrentTimestampMs();
    frame.intent = std::move(*intent_result);

    return frame;
}

std::uint64_t Engine::NextFrameId() {
    return next_frame_id_++;
}

std::uint64_t Engine::CurrentTimestampMs() const {
    const auto now = std::chrono::system_clock::now();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count());
}

std::expected<AgentIntent, WardenError>
Engine::DeserializeIntentOnly(std::string_view validated_json) const {
    try {
        const json j = json::parse(validated_json);

        if (!j.is_object()) {
            std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: root must be a JSON object.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        if (!HasOnlyKeys(j, {"intent_type", "payload"})) {
            std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: unauthorized root field detected.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        if (!j.contains("intent_type") ||
            !j.contains("payload")) {
            std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: missing required root field(s).\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        if (!j.at("intent_type").is_string()) {
            std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: intent_type must be a string.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        const std::string intent_type = j.at("intent_type").get<std::string>();
        const json& payload = j.at("payload");

        if (!payload.is_object()) {
            std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: payload must be an object.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }

        if (intent_type == "System2Think") {
            if (!HasOnlyKeys(payload, {"internal_monologue"}) ||
                !payload.contains("internal_monologue") ||
                !payload.at("internal_monologue").is_string()) {
                std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: invalid System2Think payload.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            return System2Think{
                payload.at("internal_monologue").get<std::string>()
            };
        }

        if (intent_type == "QueryMerovingian") {
            if (!HasOnlyKeys(payload, {"entity_node_id", "relation_type"}) ||
                !payload.contains("entity_node_id") ||
                !payload.contains("relation_type") ||
                !payload.at("entity_node_id").is_string() ||
                !payload.at("relation_type").is_string()) {
                std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: invalid QueryMerovingian payload.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            return QueryMerovingian{
                payload.at("entity_node_id").get<std::string>(),
                payload.at("relation_type").get<std::string>()
            };
        }

        if (intent_type == "InvokeSeraph") {
            if (!HasOnlyKeys(payload, {"target_wasm_module", "target_function", "arguments"}) ||
                !payload.contains("target_wasm_module") ||
                !payload.contains("target_function") ||
                !payload.contains("arguments") ||
                !payload.at("target_wasm_module").is_string() ||
                !payload.at("target_function").is_string()) {
                std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: invalid InvokeSeraph payload.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            std::unordered_map<std::string, std::string> args;
            if (!TryReadStringMap(payload.at("arguments"), args)) {
                std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: InvokeSeraph arguments must be a string:string map.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            return InvokeSeraph{
                payload.at("target_wasm_module").get<std::string>(),
                payload.at("target_function").get<std::string>(),
                std::move(args)
            };
        }

        if (intent_type == "BroadcastSmith") {
            if (!HasOnlyKeys(payload, {"target_agent_id", "binary_payload"}) ||
                !payload.contains("target_agent_id") ||
                !payload.contains("binary_payload") ||
                !payload.at("binary_payload").is_array()) {
                std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: invalid BroadcastSmith payload.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            std::uint64_t target_agent_id = 0;
            if (!TryReadUInt64(payload.at("target_agent_id"), target_agent_id)) {
                std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: target_agent_id must be a non-negative integer.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }

            std::vector<std::uint8_t> binary_payload;
            binary_payload.reserve(payload.at("binary_payload").size());

            for (const auto& byte_val : payload.at("binary_payload")) {
                std::uint8_t parsed_byte = 0;
                if (!TryReadByte(byte_val, parsed_byte)) {
                    std::cerr << "Engine::DeserializeIntentOnly - GrammarViolation: binary_payload must contain bytes in range 0..255.\n";
                    return std::unexpected(WardenError::GrammarViolation);
                }
                binary_payload.push_back(parsed_byte);
            }

            return BroadcastSmith{
                target_agent_id,
                std::move(binary_payload)
            };
        }

        std::cerr << "Engine::DeserializeIntentOnly - UnauthorizedIntent: unknown intent_type: "
                  << intent_type << "\n";
        return std::unexpected(WardenError::UnauthorizedIntent);

    } catch (const json::parse_error& e) {
        std::cerr << "Engine::DeserializeIntentOnly - JSON parser error: " << e.what() << "\n";
        return std::unexpected(WardenError::GrammarViolation);
    } catch (const json::type_error& e) {
        std::cerr << "Engine::DeserializeIntentOnly - JSON type error: " << e.what() << "\n";
        return std::unexpected(WardenError::GrammarViolation);
    } catch (const json::out_of_range& e) {
        std::cerr << "Engine::DeserializeIntentOnly - JSON field missing: " << e.what() << "\n";
        return std::unexpected(WardenError::GrammarViolation);
    }
}

} // namespace Architect::Warden
