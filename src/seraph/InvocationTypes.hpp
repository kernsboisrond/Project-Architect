#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Architect::Seraph {

struct CapabilityManifest {
    std::unordered_map<std::string, std::vector<std::string>> allowed_exports{};
    std::vector<std::string> allowed_fs_paths{};
    std::vector<std::string> allowed_network_hosts{};
};

struct InvocationRequest {
    std::string module_name{};
    std::string function_name{};
    std::unordered_map<std::string, std::string> arguments{};
    CapabilityManifest capabilities{};
};

struct InvocationResult {
    bool success{false};
    std::string output{};
    std::string error{};
    int status_code{0};
    std::uint64_t execution_time_ms{0};
};

} // namespace Architect::Seraph
