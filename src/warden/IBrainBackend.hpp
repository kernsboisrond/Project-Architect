#pragma once
#include <expected>
#include <string>
#include <string_view>

namespace Architect::Warden {

enum class BrainError {
    BackendUnavailable,
    GenerationFailed,
    EmptyOutput
};

class IBrainBackend {
public:
    virtual ~IBrainBackend() = default;

    virtual std::expected<std::string, BrainError>
    Generate(std::string_view prompt, std::string_view grammar) = 0;
};

} // namespace Architect::Warden
