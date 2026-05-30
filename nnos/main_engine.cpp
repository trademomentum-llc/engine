// main_engine.cpp
#include "neuro_profile.hpp"
#include "profile_matcher.hpp"
#include "task_manager.hpp"
#include "state_monitor.hpp"
#include <iostream>

using namespace nnos;

int main() {
    // Step 1: Initialize with observed behavior (in real system, from sensors/logs)
    NeuroProfile observed_profile = profiles::SYSTEMS_HYPERFOCUS;  // placeholder
    observed_profile.hyperfocus_inclination = TraitLevel::VERY_HIGH;
    observed_profile.sensory_sensitivity_level = TraitLevel::HIGH;
    
    // Step 2: Match to template profile
    ProfileMatcher matcher;
    NeuroProfile active_profile = matcher.match_profile(observed_profile);
    
    std::cout << "Matched profile ID: " << static_cast<int>(active_profile.profile_id) 
              << " with confidence: " << active_profile.confidence << "\n";
    
    // Step 3: Initialize subsystems with active profile
    TaskManager task_mgr(active_profile);
    StateMonitor state_mon(active_profile);
    
    // Step 4: Real-time loop (simplified)
    while (true) {
        // Read physiological state from neural link
        PhysiologicalState current_state = {
            .heart_rate_variability = 0.7f,
            .noise_level = 0.3f,
            .light_level = 0.5f,
            .notifications_count = 5,
            .self_report_overwhelm = 3
        };
        
        // Assess state and intervene if needed
        InterventionTier tier = state_mon.assess_and_intervene(current_state);
        
        if (tier == InterventionTier::EMERGENCY_SHUTDOWN) {
            std::cout << "Emergency shutdown triggered\n";
            // enter recovery mode
            break;
        }
        
        // Process incoming task (example)
        Task new_task = {
            .id = 1,
            .demand_flags = static_cast<uint8_t>(TaskDemand::DEEP_SYSTEMS),
            .est_effort_minutes = 60,
            .importance = 8,
            .social_cost = 2,
            .sensory_cost = 3,
            .deadline_timestamp = 0
        };
        
        if (task_mgr.intake_task(new_task)) {
            std::cout << "Task scheduled at minute: " << new_task.scheduled_minute << "\n";
        } else {
            std::cout << "Task deferred\n";
        }
        
        // In real system: sleep until next event
        break;  // exit for demo
    }
    
    return 0;
}
