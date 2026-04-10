#include "PromptAssembler.hpp"

#include <sstream>

namespace Architect::Warden {

std::string PromptAssembler::BuildPrompt(const Architect::Core::AgentContext& context) {
    std::ostringstream out;

    out << "You are the ARCHITECT cognition source.\n";
    out << "Return exactly one JSON object matching the CognitiveFrame contract.\n\n";

    out << "Allowed intent types:\n";
    out << "- System2Think\n";
    out << "- QueryMerovingian\n";
    out << "- InvokeSeraph\n";
    out << "- BroadcastSmith\n\n";

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

    out << "Return only valid JSON.\n";
    return out.str();
}

} // namespace Architect::Warden
