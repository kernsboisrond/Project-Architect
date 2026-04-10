#include "warden/WardenEngine.hpp"
#include "warden/CognitiveFrame.hpp"
#include <iostream>

using namespace Architect::Warden;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

void print_frame(const CognitiveFrame& frame) {
    std::cout << "Engine Output: Frame ID [" << frame.frame_id << "] at " << frame.timestamp_ms << "ms\n";
    std::visit(overloaded{
        [](const Architect::Warden::System2Think& t) {
            std::cout << "  -> Actions: System2Think | Monologue: '" << t.internal_monologue << "'\n";
        },
        [](const Architect::Warden::QueryMerovingian& m) {
            std::cout << "  -> Actions: QueryMerovingian | Entity: " << m.entity_node_id << " | Relation: " << m.relation_type << "\n";
        },
        [](const Architect::Warden::InvokeSeraph& s) {
            std::cout << "  -> Actions: InvokeSeraph | Wasm: " << s.target_wasm_module << "\n";
        },
        [](const Architect::Warden::BroadcastSmith& b) {
            std::cout << "  -> Actions: BroadcastSmith | Target: " << b.target_agent_id << "\n";
        }
    }, frame.intent);
}

int main() {
    Engine engine;
    engine.CompileGrammarConstraints();
    
    std::cout << "\n--- Test 1: Valid System2Think ---\n";
    std::string mock_think = R"({
        "frame_id": 1,
        "timestamp_ms": 1712200000,
        "intent_type": "System2Think",
        "payload": {
            "internal_monologue": "Let me construct a data path..."
        }
    })";
    
    auto frame1 = engine.EnforceCognition(mock_think);
    if (frame1.has_value()) {
        print_frame(*frame1);
    } else {
        std::cerr << "Failed to parse System2Think\n";
        return 1;
    }
    
    std::cout << "\n--- Test 2: Valid InvokeSeraph ---\n";
    std::string mock_invoke = R"({
        "frame_id": 3,
        "timestamp_ms": 1712200020,
        "intent_type": "InvokeSeraph",
        "payload": {
            "target_wasm_module": "math_helper.wasm",
            "target_function": "execute",
            "arguments": {
                "operation": "add",
                "a": "5",
                "b": "10"
            }
        }
    })";
    
    auto frame2 = engine.EnforceCognition(mock_invoke);
    if (frame2.has_value()) {
        print_frame(*frame2);
    } else {
        std::cerr << "Failed to parse InvokeSeraph\n";
        return 1;
    }
    
    std::cout << "\n--- Test 3: Grammar Violation (Missing Fields) ---\n";
    std::string mock_bad_grammar = R"({
        "frame_id": 2,
        "timestamp_ms": 1712200010,
        "intent_type": "QueryMerovingian",
        "payload": {
            "entity_node_id": "user_42"
        }
    })"; 
    
    auto frame3 = engine.EnforceCognition(mock_bad_grammar);
    if (frame3.has_value()) {
        std::cerr << "Expected failure, but it succeeded!\n";
        return 1;
    }

    std::cout << "\n--- Test 4: Unauthorized Intent ---\n";
    std::string mock_unauthorized = R"({
        "frame_id": 99,
        "timestamp_ms": 1712200999,
        "intent_type": "ExplodeTheInternet",
        "payload": {}
    })"; 
    
    auto frame4 = engine.EnforceCognition(mock_unauthorized);
    if (frame4.has_value()) {
        std::cerr << "Expected failure, but it succeeded!\n";
        return 1;
    }
    
    std::cout << "\n--- Test 5: Valid BroadcastSmith ---\n";
    std::string mock_broadcast = R"({
        "frame_id": 4,
        "timestamp_ms": 1712200030,
        "intent_type": "BroadcastSmith",
        "payload": {
            "target_agent_id": 99,
            "binary_payload": [72, 101, 108, 108, 111]
        }
    })";
    auto frame5 = engine.EnforceCognition(mock_broadcast);
    if (frame5.has_value()) {
        print_frame(*frame5);
    } else {
        std::cerr << "Failed to parse BroadcastSmith\n";
        return 1;
    }

    std::cout << "\n--- Test 6: Malformed JSON ---\n";
    std::string mock_malformed = R"({ "incomplete": })"; 
    auto frame6 = engine.EnforceCognition(mock_malformed);
    if (frame6.has_value()) {
        std::cerr << "Expected failure on malformed JSON, but it succeeded!\n";
        return 1;
    }

    std::cout << "\n--- Test 7: Missing target_function ---\n";
    std::string mock_missing_func = R"({
        "frame_id": 7,
        "timestamp_ms": 1712200050,
        "intent_type": "InvokeSeraph",
        "payload": {
            "target_wasm_module": "math_helper.wasm",
            "arguments": {
                "operation": "add"
            }
        }
    })";
    auto frame7 = engine.EnforceCognition(mock_missing_func);
    if (frame7.has_value()) {
        std::cerr << "Expected failure on missing target_function, but it succeeded!\n";
        return 1;
    }

    std::cout << "\n--- Test 8: Invalid byte in BroadcastSmith ---\n";
    std::string mock_invalid_byte = R"({
        "frame_id": 8,
        "timestamp_ms": 1712200060,
        "intent_type": "BroadcastSmith",
        "payload": {
            "target_agent_id": 99,
            "binary_payload": [72, 256, 108]
        }
    })";
    auto frame8 = engine.EnforceCognition(mock_invalid_byte);
    if (frame8.has_value()) {
        std::cerr << "Expected failure on invalid byte, but it succeeded!\n";
        return 1;
    }

    std::cout << "\nEngine Validation tests passed.\n";
    return 0;
}
