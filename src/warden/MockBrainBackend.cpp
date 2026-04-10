#include "MockBrainBackend.hpp"

#include <string>

namespace Architect::Warden {

std::expected<std::string, BrainError>
MockBrainBackend::Generate(std::string_view prompt, std::string_view /*grammar*/) {
    const std::string assembled(prompt);

    if (assembled.find("SERAPH REJECTION") != std::string::npos) {
        return R"({
            "frame_id": 102,
            "timestamp_ms": 1712200001,
            "intent_type": "System2Think",
            "payload": {
                "internal_monologue": "My requested action was denied. I must reformulate safely."
            }
        })";
    }

    return R"({
        "frame_id": 101,
        "timestamp_ms": 1712200000,
        "intent_type": "InvokeSeraph",
        "payload": {
            "target_wasm_module": "echo",
            "target_function": "print",
            "arguments": {
                "message": "Initiating standard response."
            }
        }
    })";
}

} // namespace Architect::Warden
