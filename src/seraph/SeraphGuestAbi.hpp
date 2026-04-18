#pragma once

#include <cstdint>
#include <string_view>

namespace Architect::Seraph::Abi {

/*
 * SERAPH GUEST ABI CONTRACT:
 * 
 * required exports:
 *   - `memory`
 *   - `seraph_alloc(i32 len) -> i32 ptr`
 *   - `seraph_free(i32 ptr, i32 len) -> void`
 *   - target function `(i32 ptr, i32 len) -> i64 packed_result`
 *
 * returned i64 constraint:
 *   - upper 32 bits = i32 ptr
 *   - lower 32 bits = i32 len
 */

constexpr std::string_view kVersion = "1.0";

constexpr std::string_view kExportAlloc = "seraph_alloc";
constexpr std::string_view kExportFree  = "seraph_free";
constexpr std::string_view kExportMemory = "memory";

// Helper to pack pointer and length into a standard i64 Wasm return payload
[[nodiscard]] constexpr std::uint64_t PackI64(std::uint32_t ptr, std::uint32_t len) noexcept {
    return (static_cast<std::uint64_t>(ptr) << 32) | static_cast<std::uint64_t>(len);
}

// Helper to unpack the returned Wasm i64 payload back into host pointers
[[nodiscard]] constexpr std::uint32_t UnpackPtr(std::uint64_t packed) noexcept {
    return static_cast<std::uint32_t>(packed >> 32);
}

[[nodiscard]] constexpr std::uint32_t UnpackLen(std::uint64_t packed) noexcept {
    return static_cast<std::uint32_t>(packed & 0xFFFFFFFF);
}

} // namespace Architect::Seraph::Abi
