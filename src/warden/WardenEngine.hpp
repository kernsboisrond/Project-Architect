#pragma once

#include "CognitiveFrame.hpp"
#include "IBrainBackend.hpp"
#include "core/AgentContext.hpp"

#include <expected>
#include <memory>
#include <string>
#include <string_view>

namespace Architect::Warden {

class Engine {
private:
    std::string active_gbnf_grammar_;
    std::unique_ptr<IBrainBackend> brain_;

public:
    explicit Engine(std::unique_ptr<IBrainBackend> brain);

    void CompileGrammarConstraints();

    [[nodiscard]]
    std::string_view ActiveGrammar() const noexcept;

    [[nodiscard]]
    std::expected<CognitiveFrame, WardenError>
    EnforceCognition(const Architect::Core::AgentContext& context);

private:
    [[nodiscard]]
    std::expected<CognitiveFrame, WardenError>
    DeserializeSIMD(std::string_view validated_json) const;
};

} // namespace Architect::Warden
