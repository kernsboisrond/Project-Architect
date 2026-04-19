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
    if (error == ExecutionError::FileNotFound) {
        return "Execution Failed: The module file could not be located. Check the spelling of the target module.";
    }
    if (error == ExecutionError::ModuleLoadError) {
        return "Execution Failed: The module bytes could not be loaded into the WebAssembly engine.";
    }
    if (error == ExecutionError::InvalidModule) {
        return "Execution Failed: Invalid module routing block path.";
    }
    if (error == ExecutionError::WasmCompileError) {
        return "Execution Failed: Target format cannot be compiled natively into Wasm representations.";
    }
    if (error == ExecutionError::MissingExport) {
        return "Execution Failed: Target did not export essential seraph memory ABIs or the required function.";
    }
    if (error == ExecutionError::GuestTrap) {
        return "Execution Failed: The WebAssembly module trapped internally resulting in an unrecoverable fault.";
    }
    if (error == ExecutionError::InvalidMemoryBounds) {
        return "Execution Failed: Parameters or outputs violated the linear memory access constraints or integer bounding boundaries.";
    }
    if (error == ExecutionError::AllocatorError) {
        return "Execution Failed: Invalid seraph_alloc instantiation.";
    }
    if (error == ExecutionError::FunctionCallError) {
        return "Execution Failed: Binding call to the internal Wasm function mismatched signatures natively.";
    }
    if (error == ExecutionError::ChecksumMismatch) {
        return "Execution Failed: WebAssembly module bytes structurally mismatch the explicit SHA256 checksum defined in the registry. Module execution blocked due to potential tampering.";
    }
    if (error == ExecutionError::UntrustedModule) {
        return "Execution Failed: WebAssembly module is identified but explicitly mapped as untrusted within the authoritative registry constraints.";
    }
    if (error == ExecutionError::PayloadTooLarge) {
        return "Execution Failed: The input or output payload exceeded the maximum permitted size limitations.";
    }
    if (error == ExecutionError::FuelExhausted) {
        return "Execution Failed: The module exceeded its permitted execution instructions (fuel exhaustion).";
    }

    return "Execution Denied: An unknown runtime error occurred during invocation.";
}

} // namespace Architect::Seraph
