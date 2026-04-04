#include "WardenEngine.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Architect::Warden {

void Engine::CompileGrammarConstraints() {
    std::cout << "Engine: Compiling strict GBNF grammar constraints for local LLM inference...\n";
    
    // Establishing the explicit GBNF grammar that will bound generation at the token level
    active_gbnf_grammar_ = R"(root ::= "{" ws "\"frame_id\":" ws number "," ws "\"timestamp_ms\":" ws number "," ws "\"intent_type\":" ws intent_type "," ws "\"payload\":" ws payload "}"
ws ::= [ \t\n]*
string ::= "\"" [^"\\]* "\"" 
number ::= [0-9]+
number_array ::= "[" ws (number (ws "," ws number)*)? ws "]"
dict ::= "{" ws (string ws ":" ws string (ws "," ws string ws ":" ws string)*)? ws "}"

intent_type ::= "\"System2Think\"" | "\"QueryMerovingian\"" | "\"InvokeSeraph\"" | "\"BroadcastSmith\""
payload ::= payload_think | payload_query | payload_invoke | payload_broadcast

payload_think ::= "{" ws "\"internal_monologue\":" ws string "}"
payload_query ::= "{" ws "\"entity_node_id\":" ws string "," ws "\"relation_type\":" ws string "}"
payload_invoke ::= "{" ws "\"target_wasm_module\":" ws string "," ws "\"arguments\":" ws dict "}"
payload_broadcast ::= "{" ws "\"target_agent_id\":" ws number "," ws "\"binary_payload\":" ws number_array "}"
)";

    std::cout << "-> Grammar loaded. Model emissions will be forcefully restricted.\n";
}

std::expected<CognitiveFrame, WardenError> Engine::EnforceCognition(std::string_view stimulus) {
    return DeserializeSIMD(stimulus);
}

std::expected<CognitiveFrame, WardenError> Engine::DeserializeSIMD(std::string_view validated_json) {
    try {
        auto j = json::parse(validated_json);
        
        CognitiveFrame frame;
        frame.frame_id = j.value("frame_id", 0ULL);
        frame.timestamp_ms = j.value("timestamp_ms", 0ULL);
        
        if (!j.contains("intent_type") || !j.contains("payload")) {
            std::cerr << "Engine::DeserializeSIMD - GrammarViolation: missing 'intent_type' or 'payload' block.\n";
            return std::unexpected(WardenError::GrammarViolation);
        }
        
        // Ensure no unauthorized ghost fields exist in root
        for (auto& el : j.items()) {
            if (el.key() != "frame_id" && el.key() != "timestamp_ms" && 
                el.key() != "intent_type" && el.key() != "payload") {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: Unauthorized root field detected: " << el.key() << "\n";
                return std::unexpected(WardenError::GrammarViolation);
            }
        }
        
        std::string intent_type = j["intent_type"];
        auto payload = j["payload"];
        
        if (intent_type == "System2Think") {
            if (payload.size() != 1 || !payload.contains("internal_monologue")) {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: Invalid fields in System2Think.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }
            frame.intent = System2Think{payload.at("internal_monologue").get<std::string>()};
        } 
        else if (intent_type == "QueryMerovingian") {
            if (payload.size() != 2 || !payload.contains("entity_node_id") || !payload.contains("relation_type")) {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: Invalid fields in QueryMerovingian.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }
            frame.intent = QueryMerovingian{
                payload.at("entity_node_id").get<std::string>(),
                payload.at("relation_type").get<std::string>()
            };
        } 
        else if (intent_type == "InvokeSeraph") {
            if (payload.size() != 2 || !payload.contains("target_wasm_module") || !payload.contains("arguments")) {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: Invalid fields in InvokeSeraph.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }
            std::unordered_map<std::string, std::string> args = payload.at("arguments").get<std::unordered_map<std::string, std::string>>();
            frame.intent = InvokeSeraph{
                payload.at("target_wasm_module").get<std::string>(),
                args
            };
        } 
        else if (intent_type == "BroadcastSmith") {
            if (payload.size() != 2 || !payload.contains("target_agent_id") || !payload.contains("binary_payload")) {
                std::cerr << "Engine::DeserializeSIMD - GrammarViolation: Invalid fields in BroadcastSmith.\n";
                return std::unexpected(WardenError::GrammarViolation);
            }
            std::vector<uint8_t> p;
            for (auto& byte_val : payload.at("binary_payload")) {
                p.push_back(byte_val.get<uint8_t>());
            }
            frame.intent = BroadcastSmith{
                payload.at("target_agent_id").get<uint64_t>(),
                p
            };
        } 
        else {
            std::cerr << "Engine::DeserializeSIMD - UnauthorizedIntent: unknown intent_type: " << intent_type << "\n";
            return std::unexpected(WardenError::UnauthorizedIntent);
        }
        
        return frame;
    } catch (const json::parse_error& e) {
        std::cerr << "Engine::DeserializeSIMD - JSON parser error: " << e.what() << "\n";
        return std::unexpected(WardenError::GrammarViolation);
    } catch (const json::type_error& e) {
        std::cerr << "Engine::DeserializeSIMD - JSON type error (incorrect schema fields): " << e.what() << "\n";
        return std::unexpected(WardenError::GrammarViolation);
    } catch (const json::out_of_range& e) {
        std::cerr << "Engine::DeserializeSIMD - JSON field missing: " << e.what() << "\n";
        return std::unexpected(WardenError::GrammarViolation);
    }
}

} // namespace Architect::Warden
