#include "seraph/ModuleRegistry.hpp"
#include <iostream>
#include <memory>
#include <fstream>
#include <string>

using namespace Architect::Seraph;

int main() {
    auto registry = std::make_shared<ModuleRegistry>();

    // Test 1: Load Valid Manifest
    auto result = registry->LoadManifest("tests/fixtures/manifest.json");
    if (!result.has_value()) {
        std::cerr << "[FAIL] Failed to load valid manifest.\n";
        return 1;
    }
    std::cout << "[PASS] Successfully loaded valid manifest.\n";

    // Test 2: Check Trust & Metadata from Registry Profile
    auto echo_mod = registry->GetModuleProfile("echo");
    if (!echo_mod || !echo_mod->trusted || echo_mod->guest_abi_version != "1.0") {
        std::cerr << "[FAIL] Failed to retrieve trusted structured profile for echo.\n";
        return 1;
    }
    std::cout << "[PASS] Retrieved valid profile mapping fields exactly.\n";

    // Test 3: Missing Profile Lookups return null safely
    auto missing = registry->GetModuleProfile("unregistered_mod");
    if (missing) {
        std::cerr << "[FAIL] Unregistered mod should yield nullptr natively.\n";
        return 1;
    }
    std::cout << "[PASS] Unregistered mapping returns correctly.\n";

    // Test 4: Manifest Missing mapping directly onto ManifestNotFound
    auto err_res = registry->LoadManifest("tests/fixtures/non_existent.json");
    if (err_res.has_value() || err_res.error() != RegistryError::ManifestNotFound) {
        std::cerr << "[FAIL] Missing manifest loading did not map ManifestNotFound properly.\n";
        return 1;
    }
    std::cout << "[PASS] Missing manifest blocks safely.\n";

    // Create a temporary broken JSON manifest ensuring ParseFailed mappings 
    std::ofstream tmp("tests/fixtures/broken.json");
    tmp << "{ \"manifest_version\": \"1\", \"modules\": { broken bytes ";
    tmp.close();

    auto err_parse = registry->LoadManifest("tests/fixtures/broken.json");
    if (err_parse.has_value() || err_parse.error() != RegistryError::ManifestParseFailed) {
        std::cerr << "[FAIL] Malformed JSON did not map ManifestParseFailed properly.\n";
        return 1;
    }
    std::cout << "[PASS] Broken JSON parsing blocks safely.\n";
    std::remove("tests/fixtures/broken.json");

    std::cout << "\nAll System Registry tests passed perfectly!\n";
    return 0;
}
