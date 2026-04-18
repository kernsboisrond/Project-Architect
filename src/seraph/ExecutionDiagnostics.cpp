#include "ExecutionDiagnostics.hpp"

namespace Architect::Seraph {

std::string GetSemanticFeedback(ExecutionError error) {
    if (error == ExecutionError::UnsupportedModule) {
        return "Execution Denied: The requested module does not exist or is not permitted. Please review available valid modules.";
    }
    if (error == ExecutionError::UnsupportedFunction) {
        return "Execution Denied: The requested function is not exported by the specified module.";
    }
    if (error == ExecutionError::CapabilityDenied) {
        return "Execution Denied: The SERAPH sandbox rejected the execution due to an unauthorized capability request.";
    }
    if (error == ExecutionError::InvalidArguments) {
        return "Execution Denied: The arguments provided to the function do not match the expected schema.";
    }
    
    return "Execution Denied: An unknown runtime error occurred during invocation.";
}

} // namespace Architect::Seraph
