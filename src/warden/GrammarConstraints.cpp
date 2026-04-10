#include "GrammarConstraints.hpp"

namespace Architect::Warden {

std::string_view GetCognitiveFrameGrammar() {
    static constexpr std::string_view grammar = R"(root ::= "{" ws "\"frame_id\":" ws number "," ws "\"timestamp_ms\":" ws number "," ws "\"intent_type\":" ws intent_type "," ws "\"payload\":" ws payload "}"
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
    return grammar;
}

} // namespace Architect::Warden
