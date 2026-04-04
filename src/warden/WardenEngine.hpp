#pragma once
#include "CognitiveFrame.hpp"
#include <expected>
#include <string>
#include <string_view>

namespace Architect::Warden {

class Engine {
private:
    std::string active_gbnf_grammar_;

public:
    Engine() = default;

    void CompileGrammarConstraints();

    [[nodiscard]]
    std::expected<CognitiveFrame, WardenError>
    EnforceCognition(std::string_view stimulus);

private:
    [[nodiscard]]
    std::expected<CognitiveFrame, WardenError>
    DeserializeSIMD(std::string_view validated_json);
};

} // namespace Architect::Warden
