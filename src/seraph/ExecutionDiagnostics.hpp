#pragma once

#include <string>

namespace Architect::Seraph {

enum class ExecutionError {
    UnsupportedModule,
    UnsupportedFunction,
    CapabilityDenied,
    InvalidArguments
};

[[nodiscard]]
std::string GetSemanticFeedback(ExecutionError error);

} // namespace Architect::Seraph
