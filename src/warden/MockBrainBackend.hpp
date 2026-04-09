#pragma once
#include "IBrainBackend.hpp"
#include <string>

namespace Architect::Warden {

class MockBrainBackend : public IBrainBackend {
public:
    MockBrainBackend() = default;
    ~MockBrainBackend() override = default;

    // Optional: Inject a specific raw JSON string to be returned on the next Generate() call
    void SetNextResponse(std::string response);

    std::expected<std::string, BrainError> Generate(
        std::string_view prompt,
        std::string_view grammar
    ) override;

private:
    std::string next_response_;
};

} // namespace Architect::Warden
