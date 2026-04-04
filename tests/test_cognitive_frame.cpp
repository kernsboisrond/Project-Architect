#include "warden/CognitiveFrame.hpp"
#include <iostream>

using namespace Architect::Warden;

int main() {
    CognitiveFrame frame;
    frame.frame_id = 1;
    frame.timestamp_ms = 1000;
    frame.intent = System2Think{"test internal monologue"};
    
    if (frame.frame_id != 1) {
        std::cerr << "CognitiveFrame instantiation failed.\n";
        return 1;
    }
    
    std::cout << "CognitiveFrame tests passed.\n";
    return 0;
}
