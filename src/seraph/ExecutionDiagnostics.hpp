#pragma once

#include <string>

namespace Architect::Seraph {

enum class ExecutionError {
    UnsupportedModule,
    UnsupportedFunction,
    CapabilityDenied,
    InvalidArguments,
    FileNotFound,
    ModuleLoadError,
    InvalidModule,
    WasmCompileError,
    MissingExport,
    GuestTrap,
    InvalidMemoryBounds,
    AllocatorError,
    FunctionCallError
};

[[nodiscard]]
std::string GetSemanticFeedback(ExecutionError error);

} // namespace Architect::Seraph
