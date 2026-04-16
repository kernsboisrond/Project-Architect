#pragma once

#include "IBrainBackend.hpp"

#include <expected>
#include <string>
#include <string_view>

namespace Architect::Warden {

struct LlamaBackendConfig {
    std::string model_path{};
    int n_ctx{1024};
    int n_predict{512};
    int n_gpu_layers{0};
    bool verbose{false};
};

class LlamaCppBackend final : public IBrainBackend {
public:
    explicit LlamaCppBackend(LlamaBackendConfig config);
    ~LlamaCppBackend();

    std::expected<std::string, BrainError>
    Generate(std::string_view prompt, std::string_view grammar) override;

    [[nodiscard]]
    bool IsReady() const noexcept;

private:
    LlamaBackendConfig config_;
    bool ready_{false};

    void* model_{nullptr};
    void* ctx_{nullptr};

    bool Initialize();
    void Shutdown();

    bool CreateContext();
    void DestroyContext();
};

} // namespace Architect::Warden
