#include "CognitiveLoop.hpp"
#include <iostream>
#include <string>

// The idiomatic visitor pattern for std::visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

CognitiveLoop::CognitiveLoop(Architect::Warden::Engine& engine) : engine_(engine) {}

void CognitiveLoop::run() {
    std::cout << "CognitiveLoop: Pseudo-operations loop starting. Awaiting JSON stimulus (type 'exit' to quit)...\n";
    
    while (true) {
        std::cout << "> ";
        std::string stimulus;
        if (!std::getline(std::cin, stimulus)) break;

        if (stimulus == "exit" || stimulus.empty()) break;

        auto result = engine_.EnforceCognition(stimulus);
        
        if (result.has_value()) {
            DispatchIntent(result.value());
        } else {
            std::cout << "Anomaly detected: Warden parsing validation failed.\n";
        }
    }
}

void CognitiveLoop::DispatchIntent(const Architect::Warden::CognitiveFrame& frame) {
    std::visit(overloaded{
        [](const Architect::Warden::System2Think& t) {
            std::cout << "System2Think: " << t.internal_monologue << "\n";
        },
        [](const Architect::Warden::QueryMerovingian& m) {
            std::cout << "QueryMerovingian: " << m.entity_node_id
                      << " / " << m.relation_type << "\n";
        },
        [](const Architect::Warden::InvokeSeraph& s) {
            std::cout << "InvokeSeraph: " << s.target_wasm_module << "\n";
        },
        [](const Architect::Warden::BroadcastSmith& b) {
            std::cout << "BroadcastSmith to agent: " << b.target_agent_id << "\n";
        }
    }, frame.intent);
}
