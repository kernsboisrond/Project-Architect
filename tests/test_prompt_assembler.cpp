#include "warden/PromptAssembler.hpp"
#include "core/AgentContext.hpp"
#include <iostream>
#include <string>

int main() {
    using namespace Architect::Warden;
    using namespace Architect::Core;

    AgentContext ctx;
    ctx.current_stimulus = "Test stimulus";
    ctx.available_capabilities = {"echo::print", "math::add"};
    ctx.memory_pointers = {"memory_1"};
    ctx.last_action_feedback = "Success";

    std::string prompt = PromptAssembler::BuildPrompt(ctx);

    auto check_contains = [&prompt](const std::string& substring, const std::string& description) {
        if (prompt.find(substring) == std::string::npos) {
            std::cerr << "[Test] FAILED: Missing " << description << " in prompt.\n";
            std::cerr << "Substring expected: " << substring << "\n";
            return false;
        }
        return true;
    };

    bool passed = true;
    passed &= check_contains("Legal root keys exactly:", "legal root key guidance");
    passed &= check_contains("Legal intent and payload pairings:", "legal intent/payload mapping");
    passed &= check_contains("Available capabilities:", "capability list");
    passed &= check_contains("Current stimulus:", "current stimulus");
    passed &= check_contains("Last action feedback:", "feedback section");
    passed &= check_contains("Never mix payload fields from different intents.", "anti-mixing rule");
    passed &= check_contains("Minimal valid examples:", "example frames");
    passed &= check_contains("Test stimulus", "inserted stimulus text");
    passed &= check_contains("echo::print", "inserted capability text");

    if (passed) {
        std::cout << "[Test] PromptAssembler formatting and content verification passed.\n";
        return 0;
    } else {
        return 1;
    }
}
