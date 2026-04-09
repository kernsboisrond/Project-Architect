#pragma once

#include <string>
#include <unordered_map>

namespace Architect::Seraph {

struct CapabilitySet {
    bool allow_fs_read{false};
    bool allow_fs_write{false};
    bool allow_network{false};
    bool allow_process_spawn{false};
};

struct InvocationRequest {
    std::string module_name{};
    std::string function_name{};
    std::unordered_map<std::string, std::string> arguments{};
    CapabilitySet capabilities{};
};

struct InvocationResult {
    bool success{false};
    std::string output{};
    std::string error{};
};

} // namespace Architect::Seraph
