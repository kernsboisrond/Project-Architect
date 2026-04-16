#include "LlamaCppBackend.hpp"

#if ARCHITECT_ENABLE_LLAMA
#include "llama.h"
#endif

#include <utility>
#include <iostream>
#include <vector>

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

    if (!CreateContext()) {
        llama_model_free(static_cast<llama_model*>(model_));
        model_ = nullptr;
        return false;
    }

    std::cout << "[LlamaCppBackend] Model loaded and initial context verified.\n";
    return true;
#else
    return false;
#endif
}

bool LlamaCppBackend::CreateContext() {
#if ARCHITECT_ENABLE_LLAMA
    if (!model_) return false;

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = config_.n_ctx;
    ctx_params.n_batch = 128;
    ctx_params.n_ubatch = 128;

    auto* ctx = llama_init_from_model(static_cast<llama_model*>(model_), ctx_params);
    if (!ctx) {
        std::cerr << "[LlamaCppBackend] Error: failed to create context.\n";
        return false;
    }
    ctx_ = ctx;
    return true;
#else
    return false;
#endif
}

void LlamaCppBackend::DestroyContext() {
#if ARCHITECT_ENABLE_LLAMA
    if (ctx_) {
        llama_free(static_cast<llama_context*>(ctx_));
        ctx_ = nullptr;
    }
#endif
}

void LlamaCppBackend::Shutdown() {
#if ARCHITECT_ENABLE_LLAMA
    DestroyContext();
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
LlamaCppBackend::Generate(std::string_view prompt, std::string_view /*grammar*/) {
#if ARCHITECT_ENABLE_LLAMA
    if (!ready_ || !model_) {
        return std::unexpected(BrainError::BackendUnavailable);
    }

    DestroyContext();
    if (!CreateContext()) {
        return std::unexpected(BrainError::BackendUnavailable);
    }

    auto* model = static_cast<llama_model*>(model_);
    auto* ctx = static_cast<llama_context*>(ctx_);

    // 1. Get vocab
    const auto* vocab = llama_model_get_vocab(model);

    // 2. Tokenize prompt
    const bool add_special = true;
    const bool parse_special = true;

    // First pass to get size
    int n_tokens = llama_tokenize(vocab, prompt.data(), prompt.length(), nullptr, 0, add_special, parse_special);
    if (n_tokens < 0) {
        n_tokens = -n_tokens;
    }

    std::vector<llama_token> prompt_tokens(n_tokens);
    int ret = llama_tokenize(vocab, prompt.data(), prompt.length(), prompt_tokens.data(), prompt_tokens.size(), add_special, parse_special);
    if (ret < 0) {
        return std::unexpected(BrainError::GenerationFailed);
    }

    // 3. Per-request context recreation provides isolation for this pinned version.

    // 4. Create a sampler chain
    auto sampler_params = llama_sampler_chain_default_params();
    llama_sampler* smpl = llama_sampler_chain_init(sampler_params);
    llama_sampler_chain_add(smpl, llama_sampler_init_greedy());

    // 5. Evaluate the prompt
    if (llama_decode(ctx, llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size()))) {
        llama_sampler_free(smpl);
        return std::unexpected(BrainError::GenerationFailed);
    }

    // 6. Generation loop
    std::string result;
    int n_predict = config_.n_predict;
    
    for (int i = 0; i < n_predict; ++i) {
        // Sample next token
        llama_token new_token_id = llama_sampler_sample(smpl, ctx, -1);
        
        // Stop on EOG
        if (llama_token_is_eog(vocab, new_token_id)) {
            break;
        }

        // Convert token to text
        char buf[128];
        int n_chars = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf), 0, parse_special);
        if (n_chars < 0) {
            llama_sampler_free(smpl);
            return std::unexpected(BrainError::GenerationFailed);
        }
        result.append(buf, n_chars);

        // Feed sampled token back for next evaluation
        if (llama_decode(ctx, llama_batch_get_one(&new_token_id, 1))) {
            llama_sampler_free(smpl);
            return std::unexpected(BrainError::GenerationFailed);
        }
    }

    // 7. Free sampler
    llama_sampler_free(smpl);

    return result;
#else
    return std::unexpected(BrainError::BackendUnavailable);
#endif
}

} // namespace Architect::Warden
