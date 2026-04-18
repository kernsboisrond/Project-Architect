#pragma once

#include "InvocationTypes.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Architect::Seraph {

struct RegisteredModule {
    std::string module_name;
    std::filesystem::path file_path;
    std::string sha256;
    bool trusted{false};
    CapabilityManifest capabilities;
    std::string guest_abi_version;
};

enum class RegistryError {
    ManifestNotFound,
    ManifestParseFailed,
    InvalidSchema,
    UnsupportedManifestVersion
};

class ModuleRegistry {
public:
    static constexpr std::string_view kSupportedManifestVersion = "1";

    ModuleRegistry() = default;

    [[nodiscard]]
    std::expected<void, RegistryError>
    LoadManifest(const std::filesystem::path& manifest_path);

    [[nodiscard]]
    const RegisteredModule* GetModuleProfile(const std::string& module_name) const;

    [[nodiscard]]
    CapabilityManifest GenerateSystemPolicy() const;

    [[nodiscard]]
    std::vector<std::string> DescribePromptCapabilities() const;

    static std::string ComputeSha256(const std::vector<uint8_t>& binary_payload);

private:
    std::string manifest_version_;
    std::unordered_map<std::string, RegisteredModule> tracked_modules_;
};

} // namespace Architect::Seraph
