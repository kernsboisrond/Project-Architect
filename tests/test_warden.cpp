#include "warden/WardenEngine.hpp"
#include "warden/CognitiveFrame.hpp"
#include "warden/IBrainBackend.hpp"
#include "core/AgentContext.hpp"
#include <iostream>
#include <memory>
#include <string>

using namespace Architect::Warden;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

class TestBrainBackend final : public IBrainBackend {
public:
    std::string mock_response;
    explicit TestBrainBackend(std::string response) : mock_response(std::move(response)) {}
    
    std::expected<std::string, BrainError> Generate(std::string_view, std::string_view) override {
        return mock_response;
    }
};

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
    Architect::Core::AgentContext dummy_context;
    dummy_context.current_stimulus = "test stimulus";

    std::cout << "\n--- Test 1: Valid System2Think ---\n";
    std::string mock_think = R"({
        "frame_id": 1,
        "timestamp_ms": 1712200000,
        "intent_type": "System2Think",
        "payload": {
            "internal_monologue": "Let me construct a data path..."
        }
    })";
    
    {
        Engine engine(std::make_unique<TestBrainBackend>(mock_think));
        auto frame1 = engine.EnforceCognition(dummy_context);
        if (frame1.has_value()) {
            print_frame(*frame1);
        } else {
            std::cerr << "Failed to parse System2Think\n";
            return 1;
        }
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
    
    {
        Engine engine(std::make_unique<TestBrainBackend>(mock_invoke));
        auto frame2 = engine.EnforceCognition(dummy_context);
        if (frame2.has_value()) {
            print_frame(*frame2);
        } else {
            std::cerr << "Failed to parse InvokeSeraph\n";
            return 1;
        }
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
    
    {
        Engine engine(std::make_unique<TestBrainBackend>(mock_bad_grammar));
        auto frame3 = engine.EnforceCognition(dummy_context);
        if (frame3.has_value()) {
            std::cerr << "Expected failure, but it succeeded!\n";
            return 1;
        }
    }

    std::cout << "\n--- Test 4: Unauthorized Intent ---\n";
    std::string mock_unauthorized = R"({
        "frame_id": 99,
        "timestamp_ms": 1712200999,
        "intent_type": "ExplodeTheInternet",
        "payload": {}
    })"; 
    
    {
        Engine engine(std::make_unique<TestBrainBackend>(mock_unauthorized));
        auto frame4 = engine.EnforceCognition(dummy_context);
        if (frame4.has_value()) {
            std::cerr << "Expected failure, but it succeeded!\n";
            return 1;
        }
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
    {
        Engine engine(std::make_unique<TestBrainBackend>(mock_broadcast));
        auto frame5 = engine.EnforceCognition(dummy_context);
        if (frame5.has_value()) {
            print_frame(*frame5);
        } else {
            std::cerr << "Failed to parse BroadcastSmith\n";
            return 1;
        }
    }

    std::cout << "\n--- Test 6: Malformed JSON ---\n";
    std::string mock_malformed = R"({ "incomplete": })"; 
    {
        Engine engine(std::make_unique<TestBrainBackend>(mock_malformed));
        auto frame6 = engine.EnforceCognition(dummy_context);
        if (frame6.has_value()) {
            std::cerr << "Expected failure on malformed JSON, but it succeeded!\n";
            return 1;
        }
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
    {
        Engine engine(std::make_unique<TestBrainBackend>(mock_missing_func));
        auto frame7 = engine.EnforceCognition(dummy_context);
        if (frame7.has_value()) {
            std::cerr << "Expected failure on missing target_function, but it succeeded!\n";
            return 1;
        }
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
    {
        Engine engine(std::make_unique<TestBrainBackend>(mock_invalid_byte));
        auto frame8 = engine.EnforceCognition(dummy_context);
        if (frame8.has_value()) {
            std::cerr << "Expected failure on invalid byte, but it succeeded!\n";
            return 1;
        }
    }

    std::cout << "\nEngine Validation tests passed.\n";
    return 0;
}
