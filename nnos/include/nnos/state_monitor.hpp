// state_monitor.hpp
#pragma once
#include "neuro_profile.hpp"
#include <cstdint>

namespace nnos {

struct PhysiologicalState {
    uint8_t heart_rate_variability;  // 0-255, normalized
    uint8_t noise_level;             // 0-255
    uint8_t light_level;             // 0-255
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

    // NNOS_SENSORY_REGULATION procedure (integer-only hot path)
    InterventionTier assess_and_intervene(const PhysiologicalState& state) noexcept {
        uint16_t sensory_load = compute_sensory_load(state);
        uint16_t emotional_load = compute_emotional_load(state);
        uint16_t total_load = (sensory_load + emotional_load) / 2;

        uint16_t threshold = (static_cast<uint16_t>(profile_.thresholds.sensory_alert_threshold) * 100) / 255;

        if (total_load < (threshold * 60) / 100) {
            return InterventionTier::NONE;
        } else if (total_load < (threshold * 90) / 100) {
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

    // Exposed for observability / logging
    uint16_t compute_sensory_load(const PhysiologicalState& state) const noexcept {
        // Convert 0-255 inputs to 0-100 scale
        uint16_t noise = (static_cast<uint16_t>(state.noise_level) * 100) / 255;
        uint16_t light = (static_cast<uint16_t>(state.light_level) * 100) / 255;
        uint16_t sensitivity = static_cast<uint16_t>(profile_.sensory_sensitivity_level) * 25;

        uint16_t load = 0;
        load += (noise * sensitivity * 40) / 10000;
        load += (light * sensitivity * 30) / 10000;
        load += (state.notifications_count > 100 ? 100 : static_cast<uint16_t>(state.notifications_count)) * 30 / 100;

        return (load > 100) ? 100 : load;
    }

    uint16_t compute_emotional_load(const PhysiologicalState& state) const noexcept {
        uint16_t hrv = (static_cast<uint16_t>(state.heart_rate_variability) * 100) / 255;
        uint16_t overwhelm = static_cast<uint16_t>(state.self_report_overwhelm) * 10;

        uint16_t load = 0;
        load += ((100 - hrv) * 50) / 100;
        load += (overwhelm * 50) / 100;

        return (load > 100) ? 100 : load;
    }

private:
    const NeuroProfile& profile_;

    void apply_tier1(const PhysiologicalState& state) const noexcept {
        (void)state;
        // Tier 1: Environmental adjustments
    }

    void apply_tier2(const PhysiologicalState& state) const noexcept {
        (void)state;
        // Tier 2: Guided regulation
    }

    void apply_tier3(const PhysiologicalState& state) const noexcept {
        (void)state;
        // Tier 3: Emergency protocol
    }
};

} // namespace nnos
