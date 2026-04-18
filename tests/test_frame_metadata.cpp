#include "warden/WardenEngine.hpp"
#include "warden/MockBrainBackend.hpp"
#include "core/AgentContext.hpp"

#include <iostream>
#include <string>
#include <memory>

int main() {
    using namespace Architect::Warden;
    using namespace Architect::Core;

    std::cout << "[Test] testing purely kernel metadata ownership.\n";
    auto backend = std::make_unique<MockBrainBackend>();

    Engine engine{std::move(backend)};
    engine.CompileGrammarConstraints();

    AgentContext ctx;
    ctx.current_stimulus = "Test 1";
    
    std::cout << "[Test] Calling EnforceCognition 1 time...\n";
    auto result1 = engine.EnforceCognition(ctx);
    if (!result1.has_value()) {
        std::cerr << "[Test] FAILED: EnforceCognition rejected valid output.\n";
        return 1;
    }

    if (result1->frame_id == 0) {
        std::cerr << "[Test] FAILED: frame_id is 0.\n";
        return 1;
    }

    if (result1->timestamp_ms == 0) {
        std::cerr << "[Test] FAILED: timestamp_ms is 0.\n";
        return 1;
    }

    std::cout << "[Test] Calling EnforceCognition 2 times...\n";
    auto result2 = engine.EnforceCognition(ctx);
    if (!result2.has_value()) {
        std::cerr << "[Test] FAILED: EnforceCognition rejected valid output.\n";
        return 1;
    }

    if (result2->frame_id <= result1->frame_id) {
        std::cerr << "[Test] FAILED: frame_id did not increase.\n";
        return 1;
    }

    if (result2->timestamp_ms < result1->timestamp_ms) {
        std::cerr << "[Test] FAILED: timestamp_ms went backwards.\n";
        return 1;
    }

    std::cout << "[Test] PASS: Kernel metadata sequencing is working correctly.\n";
    return 0;
}
