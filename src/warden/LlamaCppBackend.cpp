#include "LlamaCppBackend.hpp"

#if ARCHITECT_ENABLE_LLAMA
#include "llama.h"
#endif

#include <utility>
#include <iostream>

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
        std::cerr << "[LlamaCppBackend] Error: model_path is empty.\n";
        return false;
    }

    llama_backend_init();

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = config_.n_gpu_layers;

    std::cout << "[LlamaCppBackend] Loading model from: " << config_.model_path << "...\n";
    auto* model = llama_model_load_from_file(config_.model_path.c_str(), model_params);
    if (!model) {
        std::cerr << "[LlamaCppBackend] Error: failed to load model.\n";
        return false;
    }
    model_ = model;

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = config_.n_ctx;

    auto* ctx = llama_init_from_model(static_cast<llama_model*>(model_), ctx_params);
    if (!ctx) {
        std::cerr << "[LlamaCppBackend] Error: failed to create context.\n";
        llama_model_free(static_cast<llama_model*>(model_));
        model_ = nullptr;
        return false;
    }
    ctx_ = ctx;

    std::cout << "[LlamaCppBackend] Model loaded and context created successfully.\n";
    return true;
#else
    return false;
#endif
}

void LlamaCppBackend::Shutdown() {
#if ARCHITECT_ENABLE_LLAMA
    if (ctx_) {
        llama_free(static_cast<llama_context*>(ctx_));
        ctx_ = nullptr;
    }
    if (model_) {
        llama_model_free(static_cast<llama_model*>(model_));
        model_ = nullptr;
    }
    llama_backend_free();
    std::cout << "[LlamaCppBackend] Shutdown complete.\n";
#endif
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
