#pragma once

#include "CognitiveFrame.hpp"
#include "IBrainBackend.hpp"
#include "core/AgentContext.hpp"

#include <cstdint>
#include <expected>
#include <memory>
#include <string>
#include <string_view>

namespace Architect::Warden {

class Engine {
private:
    std::string active_gbnf_grammar_;
    std::unique_ptr<IBrainBackend> brain_;
    std::uint64_t next_frame_id_{1};

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
    std::uint64_t NextFrameId();

    [[nodiscard]]
    std::uint64_t CurrentTimestampMs() const;

    [[nodiscard]]
    std::expected<AgentIntent, WardenError>
    DeserializeIntentOnly(std::string_view validated_json) const;
};

} // namespace Architect::Warden
