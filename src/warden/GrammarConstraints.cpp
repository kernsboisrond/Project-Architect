#include "GrammarConstraints.hpp"

namespace Architect::Warden {

std::string_view GetCognitiveFrameGrammar() {
    static constexpr std::string_view grammar = R"(root ::= "{" ws "\"intent_type\":" ws intent-type "," ws "\"payload\":" ws payload "}"
ws ::= [ \t\n\r]*
string ::= "\"" chars "\""
chars ::= "" | char chars
char ::= [^"\\] | "\\" escape
escape ::= "\"" | "\\" | "/" | "b" | "f" | "n" | "r" | "t"
number ::= [0-9]+
number-array ::= "[" ws (number (ws "," ws number)*)? ws "]"
dict ::= "{" ws (string ws ":" ws string (ws "," ws string ws ":" ws string)*)? ws "}"

intent-type ::= "\"System2Think\"" | "\"QueryMerovingian\"" | "\"InvokeSeraph\"" | "\"BroadcastSmith\""
payload ::= payload-think | payload-query | payload-invoke | payload-broadcast

payload-think ::= "{" ws "\"internal_monologue\":" ws string ws "}"
payload-query ::= "{" ws "\"entity_node_id\":" ws string ws "," ws "\"relation_type\":" ws string ws "}"
payload-invoke ::= "{" ws "\"target_wasm_module\":" ws string ws "," ws "\"target_function\":" ws string ws "," ws "\"arguments\":" ws dict ws "}"
payload-broadcast ::= "{" ws "\"target_agent_id\":" ws number ws "," ws "\"binary_payload\":" ws number-array ws "}"
)";
    return grammar;
}

} // namespace Architect::Warden
