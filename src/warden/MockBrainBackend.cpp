#include "MockBrainBackend.hpp"

namespace Architect::Warden {

void MockBrainBackend::SetNextResponse(std::string response) {
    next_response_ = std::move(response);
}

std::expected<std::string, BrainError> MockBrainBackend::Generate(
    std::string_view /*prompt*/,
    std::string_view /*grammar*/) {
    
    // If a specific test response was injected, return it once
    if (!next_response_.empty()) {
        std::string res = std::move(next_response_);
        next_response_.clear();
        return res;
    }

    // Default valid fallback frame ensuring parsing success
    return R"({
  "frame_id": 1,
  "timestamp_ms": 1700000000000,
  "intent_type": "System2Think",
  "payload": {
    "internal_monologue": "Mock backend functioning securely under simulated grammar."
  }
})";
}

} // namespace Architect::Warden
