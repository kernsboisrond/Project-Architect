#include "LlamaCppBackend.hpp"

#include <utility>

namespace Architect::Warden {

LlamaCppBackend::LlamaCppBackend(LlamaBackendConfig config)
    : config_(std::move(config)) {
    ready_ = Initialize();
}

LlamaCppBackend::~LlamaCppBackend() {
    Shutdown();
}

bool LlamaCppBackend::IsReady() const noexcept {
    return ready_;
}

bool LlamaCppBackend::Initialize() {
#if ARCHITECT_ENABLE_LLAMA
    if (config_.model_path.empty()) {
        return false;
    }

    // Real llama.cpp model loading comes in the next step.
    return false;
#else
    return false;
#endif
}

void LlamaCppBackend::Shutdown() {
#if ARCHITECT_ENABLE_LLAMA
    // Real cleanup later
#endif
    model_ = nullptr;
    ctx_ = nullptr;
    ready_ = false;
}

std::expected<std::string, BrainError>
LlamaCppBackend::Generate(std::string_view /*prompt*/, std::string_view /*grammar*/) {
#if ARCHITECT_ENABLE_LLAMA
    if (!ready_) {
        return std::unexpected(BrainError::BackendUnavailable);
    }

    return std::unexpected(BrainError::GenerationFailed);
#else
    return std::unexpected(BrainError::BackendUnavailable);
#endif
}

} // namespace Architect::Warden
