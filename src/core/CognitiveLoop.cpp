#include "CognitiveLoop.hpp"

#include <iostream>
#include <string>
#include <utility>

// Idiomatic visitor helper for std::visit
template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

CognitiveLoop::CognitiveLoop(Architect::Warden::Engine& engine, Architect::Seraph::ExecutorStub& executor)
    : engine_(engine), executor_(executor) {}

void CognitiveLoop::run() {
    std::cout
        << "CognitiveLoop: Pseudo-operations loop starting.\n"
        << "Awaiting JSON CognitiveFrame input (type 'exit' to quit)...\n";

    while (true) {
        std::cout << "> ";

        std::string stimulus;
        if (!std::getline(std::cin, stimulus)) {
            std::cout << "\nCognitiveLoop: Input stream closed. Shutting down.\n";
            break;
        }

        if (stimulus == "exit") {
            std::cout << "CognitiveLoop: Exit directive received. Shutting down.\n";
            break;
        }

        if (stimulus.empty()) {
            std::cout << "CognitiveLoop: Empty stimulus ignored.\n";
            continue;
        }

        auto result = engine_.EnforceCognition(stimulus);

        if (!result.has_value()) {
            std::cout << "Anomaly detected: Warden parsing validation failed.\n";
            continue;
        }

        DispatchIntent(result.value());
    }
}

void CognitiveLoop::DispatchIntent(
    const Architect::Warden::CognitiveFrame& frame
) const {
    using namespace Architect::Warden;

    std::cout
        << "[Frame accepted] id=" << frame.frame_id
        << " ts=" << frame.timestamp_ms << "\n";

    std::visit(
        overloaded{
            [](const System2Think& t) {
                std::cout
                    << "[System2Think]\n"
                    << "internal_monologue: "
                    << t.internal_monologue << "\n";
            },

            [](const QueryMerovingian& m) {
                std::cout
                    << "[QueryMerovingian]\n"
                    << "entity_node_id: " << m.entity_node_id << "\n"
                    << "relation_type: " << m.relation_type << "\n"
                    << "status: not implemented yet\n";
            },

            [this](const InvokeSeraph& s) {
                std::cout
                    << "[InvokeSeraph]\n"
                    << "target_wasm_module: " << s.target_wasm_module << "\n"
                    << "target_function: " << s.target_function << "\n";

                if (s.arguments.empty()) {
                    std::cout << "arguments: <none>\n";
                } else {
                    std::cout << "arguments:\n";
                    for (const auto& [key, value] : s.arguments) {
                        std::cout << "  - " << key << " = " << value << "\n";
                    }
                }

                Architect::Seraph::InvocationRequest req;
                req.module_name = s.target_wasm_module;
                req.function_name = s.target_function;
                req.arguments = s.arguments;
                // capabilities are correctly zero-initialized to false

                auto res = executor_.Execute(req);
                if (res.has_value()) {
                    std::cout << "status: SERAPH EXECUTION SUCCESS. Output: " << res->output << "\n";
                } else {
                    std::cout << "status: SERAPH EXECUTION FAILED. Code: " << static_cast<int>(res.error()) << "\n";
                }
            },

            [](const BroadcastSmith& b) {
                std::cout
                    << "[BroadcastSmith]\n"
                    << "target_agent_id: " << b.target_agent_id << "\n"
                    << "binary_payload_size: " << b.binary_payload.size() << "\n"
                    << "status: not implemented yet\n";
            }
        },
        frame.intent
    );
}
