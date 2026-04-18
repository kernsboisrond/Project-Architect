#include "ModuleRegistry.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cstring>

namespace Architect::Seraph {

namespace { // Vendored minimal SHA-256 portable helper

const uint32_t SHA256_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

};

std::string ModuleRegistry::ComputeSha256(const std::vector<uint8_t>& binary_payload) {
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    size_t length = binary_payload.size();
    size_t bit_len = length * 8;
    
    // Padding logic: msg + 0x80 + 0x00s + size (in bits as 64-bit BigEndian)
    size_t padded_length = length + 1 + 8;
    size_t padding_zeros = (64 - (padded_length % 64)) % 64;
    padded_length += padding_zeros;

    std::vector<uint8_t> buffer;
    buffer.reserve(padded_length);
    buffer.insert(buffer.end(), binary_payload.begin(), binary_payload.end());
    buffer.push_back(0x80);
    for (size_t i = 0; i < padding_zeros; i++) buffer.push_back(0);
    
    // Append 64-bit length (big-endian)
    for (int i = 7; i >= 0; i--) {
        buffer.push_back((bit_len >> (i * 8)) & 0xff);
    }

    for (size_t offset = 0; offset < padded_length; offset += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = (buffer[offset + i*4] << 24) |
                   (buffer[offset + i*4 + 1] << 16) |
                   (buffer[offset + i*4 + 2] << 8) |
                   (buffer[offset + i*4 + 3]);
        }
        for (int i = 16; i < 64; i++) {
            w[i] = SIG1(w[i - 2]) + w[i - 7] + SIG0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = state[0], b = state[1], c = state[2], d = state[3],
                 e = state[4], f = state[5], g = state[6], h = state[7];

        for (int i = 0; i < 64; i++) {
            uint32_t temp1 = h + EP1(e) + CH(e, f, g) + SHA256_K[i] + w[i];
            uint32_t temp2 = EP0(a) + MAJ(a, b, c);
            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }

        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

    std::stringstream ss;
    for (int i = 0; i < 8; i++) {
        ss << std::hex << std::setw(8) << std::setfill('0') << state[i];
    }
    return ss.str();
}

std::expected<void, RegistryError>
ModuleRegistry::LoadManifest(const std::filesystem::path& manifest_path) {
    std::ifstream file(manifest_path);
    if (!file.is_open()) return std::unexpected(RegistryError::ManifestNotFound);

    nlohmann::json root;
    try {
        file >> root;
    } catch (...) {
        return std::unexpected(RegistryError::ManifestParseFailed);
    }

    if (!root.contains("manifest_version") || !root["manifest_version"].is_string()) {
        return std::unexpected(RegistryError::InvalidSchema);
    }
    manifest_version_ = root["manifest_version"].get<std::string>();

    if (manifest_version_ != kSupportedManifestVersion) {
        return std::unexpected(RegistryError::UnsupportedManifestVersion);
    }

    if (!root.contains("modules") || !root["modules"].is_object()) {
        return std::unexpected(RegistryError::InvalidSchema);
    }

    tracked_modules_.clear();

    for (const auto& [module_name, data] : root["modules"].items()) {
        if (!data.is_object()) return std::unexpected(RegistryError::InvalidSchema);

        RegisteredModule RM;
        RM.module_name = module_name;

        if (data.contains("path") && data["path"].is_string()) {
            auto p = std::filesystem::path(data["path"].get<std::string>());
            if (p.is_absolute()) {
                RM.file_path = p;
            } else {
                RM.file_path = manifest_path.parent_path() / p;
            }
        } else {
            return std::unexpected(RegistryError::InvalidSchema);
        }

        if (data.contains("sha256") && data["sha256"].is_string()) {
            RM.sha256 = data["sha256"].get<std::string>();
        } else {
            return std::unexpected(RegistryError::InvalidSchema);
        }

        if (data.contains("trusted") && data["trusted"].is_boolean()) {
            RM.trusted = data["trusted"].get<bool>();
        }

        if (data.contains("guest_abi_version") && data["guest_abi_version"].is_string()) {
            RM.guest_abi_version = data["guest_abi_version"].get<std::string>();
        } else {
            return std::unexpected(RegistryError::InvalidSchema);
        }

        if (data.contains("allowed_exports") && data["allowed_exports"].is_array()) {
            for (const auto& exp : data["allowed_exports"]) {
                if (!exp.is_string()) return std::unexpected(RegistryError::InvalidSchema);
                RM.capabilities.allowed_exports[module_name].push_back(exp.get<std::string>());
            }
        }

        tracked_modules_[module_name] = RM;
    }

    return {};
}

const RegisteredModule* ModuleRegistry::GetModuleProfile(const std::string& module_name) const {
    auto it = tracked_modules_.find(module_name);
    if (it != tracked_modules_.end()) {
        return &(it->second);
    }
    return nullptr;
}

} // namespace Architect::Seraph
