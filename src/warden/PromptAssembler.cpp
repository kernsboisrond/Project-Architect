#include "PromptAssembler.hpp"

#include <sstream>

namespace Architect::Warden {

std::string PromptAssembler::BuildPrompt(const Architect::Core::AgentContext& context) {
    std::ostringstream out;

    out << "You are the ARCHITECT cognition source.\n";
    out << "Return exactly one JSON object matching the CognitiveFrame contract.\n";
    out << "Do not output markdown, prose, or wrapper objects.\n\n";

    out << "Legal root keys exactly:\n";
    out << "- intent_type\n";
    out << "- payload\n";
    out << "(The kernel assigns frame_id and timestamp_ms. Return ONLY intent_type and payload.)\n\n";

    out << "Legal intent and payload pairings:\n";
    out << "- System2Think => payload must be {\"internal_monologue\":\"...\"}\n";
    out << "- QueryMerovingian => payload must be {\"entity_node_id\":\"...\",\"relation_type\":\"...\"}\n";
    out << "- InvokeSeraph => payload must be {\"target_wasm_module\":\"...\",\"target_function\":\"...\",\"arguments\":{\"key\":\"value\"}}\n";
    out << "- BroadcastSmith => payload must be {\"target_agent_id\":1,\"binary_payload\":[1,2,3]}\n\n";

    out << "Critical rules:\n";
    out << "- Never mix payload fields from different intents.\n";
    out << "- Never invent wrapper keys like cognitive_frame.\n";
    out << "- Prefer the shortest valid payload.\n";
    out << "- Avoid BroadcastSmith unless explicit binary agent communication is requested.\n";
    out << "- If the user asks to print, say, or output text, YOU MUST use InvokeSeraph.\n";
    out << "- If echo::print is available, use it for output. The argument key MUST be \"message\".\n";
    out << "- For echo::print, set \"target_wasm_module\" to \"echo\" and \"target_function\" to \"print\".\n";
    out << "- Avoid System2Think when an explicit executable action is requested.\n";
    out << "- If only private reasoning is needed (no output requested), prefer System2Think.\n";
    out << "- If the request is a relation/entity lookup, prefer QueryMerovingian.\n\n";

    out << "Minimal valid examples:\n";
    out << "{\"intent_type\":\"System2Think\",\"payload\":{\"internal_monologue\":\"I should think before acting.\"}}\n";
    out << "{\"intent_type\":\"InvokeSeraph\",\"payload\":{\"target_wasm_module\":\"echo\",\"target_function\":\"print\",\"arguments\":{\"message\":\"Hello\"}}}\n";
    out << "{\"intent_type\":\"InvokeSeraph\",\"payload\":{\"target_wasm_module\":\"echo\",\"target_function\":\"print\",\"arguments\":{\"message\":\"say hello safely\"}}}\n";
    out << "{\"intent_type\":\"QueryMerovingian\",\"payload\":{\"entity_node_id\":\"hello\",\"relation_type\":\"greeting\"}}\n\n";

    out << "Available capabilities:\n";
    if (context.available_capabilities.empty()) {
        out << "- <none>\n";
    } else {
        for (const auto& capability : context.available_capabilities) {
            out << "- " << capability << "\n";
        }
    }

    out << "\nMemory pointers:\n";
    if (context.memory_pointers.empty()) {
        out << "- <none>\n";
    } else {
        for (const auto& pointer : context.memory_pointers) {
            out << "- " << pointer << "\n";
        }
    }

    out << "\nLast action feedback:\n";
    if (context.last_action_feedback.has_value()) {
        out << "- " << *context.last_action_feedback << "\n";
    } else {
        out << "- <none>\n";
    }

    out << "\nCurrent stimulus:\n";
    out << context.current_stimulus << "\n\n";

    out << "Return only one valid CognitiveFrame JSON object.\n";
    return out.str();
}

} // namespace Architect::Warden
