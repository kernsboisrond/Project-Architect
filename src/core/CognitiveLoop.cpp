#include "CognitiveLoop.hpp"

#include <iostream>
#include <string>
#include <variant>

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace {
constexpr std::size_t kMaxRecoveryAttempts = 2;
}

CognitiveLoop::CognitiveLoop(Architect::Warden::Engine& engine,
                             Architect::Seraph::IExecutor& executor,
                             Architect::Seraph::IAuditSink& audit,
                             const Architect::Seraph::CapabilityManifest& system_capabilities,
                             std::vector<std::string> prompt_capabilities)
    : engine_(engine), executor_(executor), audit_(audit), 
      system_capabilities_(system_capabilities), prompt_capabilities_(std::move(prompt_capabilities)) {}

void CognitiveLoop::run() {
    std::cout << "[ARCHITECT] Autonomous Kernel Online.\n";

    Architect::Core::AgentContext context;
    context.available_capabilities = prompt_capabilities_;

    while (true) {
        if (!context.last_action_feedback.has_value()) {
            std::cout << "\n> ";
            if (!std::getline(std::cin, context.current_stimulus)) {
                std::cout << "\n[Kernel] Input stream closed. Shutting down.\n";
                break;
            }

            if (context.current_stimulus == "exit") {
                std::cout << "[Kernel] Exit directive received. Shutting down.\n";
                break;
            }

            if (context.current_stimulus.empty()) {
                std::cout << "[Kernel] Empty stimulus ignored.\n";
                continue;
            }
        } else {
            context.current_stimulus = "SYSTEM ALERT: revise your previous action";
            std::cout << "[Kernel] Autonomous error recovery triggered...\n";
        }

        auto frame = engine_.EnforceCognition(context);
        if (!frame.has_value()) {
            context.last_action_feedback = "WARDEN ERROR: grammar or inference failure";
            ++context.recovery_attempts;

            if (context.recovery_attempts > kMaxRecoveryAttempts) {
                std::cout << "[Kernel] Recovery limit exceeded after Warden failure. Returning to input mode.\n";
                context.last_action_feedback.reset();
                context.recovery_attempts = 0;
            }

            continue;
        }

        const auto outcome = DispatchIntent(*frame);

        if (!outcome.success) {
            context.last_action_feedback = outcome.error_message;
            ++context.recovery_attempts;

            if (context.recovery_attempts > kMaxRecoveryAttempts) {
                std::cout << "[Kernel] Recovery limit exceeded. Returning to input mode.\n";
                context.last_action_feedback.reset();
                context.recovery_attempts = 0;
            }

            continue;
        }

        context.last_action_feedback.reset();
        context.recovery_attempts = 0;
    }
}

DispatchOutcome CognitiveLoop::DispatchIntent(
    const Architect::Warden::CognitiveFrame& frame
) const {
    using namespace Architect::Warden;

    std::cout
        << "[Frame accepted] id=" << frame.frame_id
        << " ts=" << frame.timestamp_ms << "\n";

    return std::visit(
        overloaded{
            [](const System2Think& t) -> DispatchOutcome {
                std::cout
                    << "[System2Think]\n"
                    << "internal_monologue: "
                    << t.internal_monologue << "\n";
                return {true, {}};
            },

            [](const QueryMerovingian& m) -> DispatchOutcome {
                std::cout
                    << "[QueryMerovingian]\n"
                    << "entity_node_id: " << m.entity_node_id << "\n"
                    << "relation_type: " << m.relation_type << "\n"
                    << "status: not implemented yet\n";
                return {false, "MEROVINGIAN unavailable"};
            },

            [this](const InvokeSeraph& s) -> DispatchOutcome {
                std::cout
                    << "[InvokeSeraph]\n"
                    << "target_wasm_module: " << s.target_wasm_module << "\n"
                    << "target_function: " << s.target_function << "\n";

                Architect::Seraph::InvocationRequest req;
                req.module_name = s.target_wasm_module;
                req.function_name = s.target_function;
                req.arguments = s.arguments;
                req.capabilities = system_capabilities_;

                const auto result = executor_.Execute(req, audit_);
                if (!result.has_value()) {
                    std::cout << "status: SERAPH EXECUTION FAILED.\n"
                              << "reason: " << Architect::Seraph::GetSemanticFeedback(result.error()) << "\n";
                    return {false, Architect::Seraph::GetSemanticFeedback(result.error())};
                }

                std::cout
                    << "status: SERAPH EXECUTION SUCCESS.\n"
                    << "output: " << result->output << "\n";
                return {true, {}};
            },

            [](const BroadcastSmith& b) -> DispatchOutcome {
                std::cout
                    << "[BroadcastSmith]\n"
                    << "target_agent_id: " << b.target_agent_id << "\n"
                    << "binary_payload_size: " << b.binary_payload.size() << "\n"
                    << "status: not implemented yet\n";
                return {false, "SMITH broadcast unavailable"};
            }
        },
        frame.intent
    );
}
