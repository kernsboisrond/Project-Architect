#pragma once
#include "IBrainBackend.hpp"

namespace Architect::Warden {

class MockBrainBackend final : public IBrainBackend {
public:
    std::expected<std::string, BrainError>
    Generate(std::string_view prompt, std::string_view grammar) override;
};

} // namespace Architect::Warden
