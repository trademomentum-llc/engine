// boot.cpp (or main_engine.cpp)
#include "neuro_profile.hpp"
#include "profile_matcher.hpp"
#include "task_manager.hpp"
#include "state_monitor.hpp"
#include "context_gating.hpp"
#include "communication_bridge.hpp"
#include <iostream>

using namespace nnos;

int main() {
    // 1. Observe initial traits (simplified stub here)
    NeuroProfile observed = profiles::SYSTEMS_HYPERFOCUS;
    // ...populate observed traits from calibration / logs...

    // 2. Match to archetype
    ProfileMatcher matcher;
    NeuroProfile active_profile = matcher.match_profile(observed);

    std::cout << "Active profile: " 
              << static_cast<int>(active_profile.profile_id)
              << " confidence=" << active_profile.confidence << "\n";

    // 3. Construct all deterministic subsystems
    TaskManager       task_mgr(active_profile);
    StateMonitor      state_mon(active_profile);
    ContextGating     ctx_gate(active_profile);
    CommunicationBridge comm_bridge(active_profile);

    // 4. From here, your event loop calls:
    //    - task_mgr.intake_task(...)
    //    - state_mon.assess_and_intervene(...)
    //    - ctx_gate.evaluate(...)
    //    - comm_bridge.evaluate(...), comm_bridge.is_likely_masking(...)

    return 0;
}
