// state_monitor.hpp
#pragma once
#include "neuro_profile.hpp"
#include <cstdint>

namespace nnos {

struct PhysiologicalState {
    float heart_rate_variability;  // 0.0-1.0, normalized
    float noise_level;             // 0.0-1.0
    float light_level;             // 0.0-1.0
    uint16_t notifications_count;
    uint8_t self_report_overwhelm;  // 0-10
} __attribute__((packed));

enum class InterventionTier : uint8_t {
    NONE = 0,
    ADJUST_ENVIRONMENT = 1,
    GUIDED_REGULATION = 2,
    EMERGENCY_SHUTDOWN = 3
};

class StateMonitor {
public:
    explicit StateMonitor(const NeuroProfile& profile)
        : profile_(profile) {}
    
    // NNOS_SENSORY_REGULATION procedure
    InterventionTier assess_and_intervene(const PhysiologicalState& state) noexcept {
        float sensory_load = compute_sensory_load(state);
        float emotional_load = compute_emotional_load(state);
        float total_load = (sensory_load + emotional_load) / 2.0f;
        
        const float threshold = profile_.thresholds.sensory_alert_threshold;
        
        if (total_load < 0.6f * threshold) {
            return InterventionTier::NONE;
        } else if (total_load < 0.9f * threshold) {
            apply_tier1(state);
            return InterventionTier::ADJUST_ENVIRONMENT;
        } else if (total_load < threshold) {
            apply_tier2(state);
            return InterventionTier::GUIDED_REGULATION;
        } else {
            apply_tier3(state);
            return InterventionTier::EMERGENCY_SHUTDOWN;
        }
    }
    
private:
    const NeuroProfile& profile_;
    
    float compute_sensory_load(const PhysiologicalState& state) const noexcept {
        float load = 0.0f;
        
        // Weight factors based on sensory sensitivity level
        float sensitivity_multiplier = static_cast<float>(profile_.sensory_sensitivity_level) / 4.0f;
        
        load += state.noise_level * sensitivity_multiplier * 0.4f;
        load += state.light_level * sensitivity_multiplier * 0.3f;
        load += (state.notifications_count / 100.0f) * 0.3f;  // cap at 100
        
        return load > 1.0f ? 1.0f : load;
    }
    
    float compute_emotional_load(const PhysiologicalState& state) const noexcept {
        float load = 0.0f;
        
        load += (1.0f - state.heart_rate_variability) * 0.5f;
        load += (state.self_report_overwhelm / 10.0f) * 0.5f;
        
        return load > 1.0f ? 1.0f : load;
    }
    
    void apply_tier1(const PhysiologicalState& state) const noexcept {
        // Tier 1: Environmental adjustments
        // In real implementation, these would trigger hardware/OS calls
        // reduce_brightness();
        // reduce_volume();
        // disable_non_critical_notifications();
    }
    
    void apply_tier2(const PhysiologicalState& state) const noexcept {
        // Tier 2: Guided regulation
        // trigger_breathing_exercise();
        // suggest_movement_break();
        // enable_stimming_mode();
    }
    
    void apply_tier3(const PhysiologicalState& state) const noexcept {
        // Tier 3: Emergency protocol
        // suspend_all_non_critical_tasks();
        // notify_trusted_contact();
        // log_context_for_recovery();
    }
};

} // namespace nnos
