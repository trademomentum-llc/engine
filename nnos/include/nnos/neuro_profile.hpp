// neuro_profile.hpp
#pragma once
#include <cstdint>
#include <array>
#include <bitset>

namespace nnos {

// Trait levels as compile-time constants
enum class TraitLevel : uint8_t {
    VERY_LOW = 0,
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    VERY_HIGH = 4
};

// Strength/vulnerability categories as bitflags
enum class StrengthFlag : uint32_t {
    DEEP_SYSTEMS_THINKING     = 1 << 0,
    PATTERN_DETECTION         = 1 << 1,
    CREATIVE_DIVERGENT        = 1 << 2,
    EMPATHY_INTEROCEPTION     = 1 << 3,
    FAST_CONTEXT_SCANNING     = 1 << 4,
    HIGH_FIDELITY_WORK        = 1 << 5,
    VISION_SETTING            = 1 << 6,
    PERSUASIVE_COMMUNICATION  = 1 << 7
};

enum class VulnerabilityFlag : uint32_t {
    CONTEXT_SWITCH_OVERLOAD   = 1 << 0,
    SENSORY_OVERLOAD          = 1 << 1,
    INITIATION_DIFFICULTY     = 1 << 2,
    COMPLETION_DIFFICULTY     = 1 << 3,
    SOCIAL_BURNOUT            = 1 << 4,
    TIME_BLINDNESS            = 1 << 5,
    SHUTDOWN_RISK             = 1 << 6,
    BURNOUT_CYCLES            = 1 << 7,
    MOOD_VARIABILITY          = 1 << 8
};

// Profile structure - fixed size, cache-friendly
struct NeuroProfile {
    // Trait dimensions
    TraitLevel need_for_structure;
    TraitLevel novelty_seeking;
    TraitLevel hyperfocus_inclination;
    TraitLevel sensory_sensitivity_level;
    TraitLevel social_energy_capacity;
    TraitLevel executive_function_difficulty;
    
    // Bitflags for O(1) membership tests
    uint32_t strengths;         // bitwise OR of StrengthFlag
    uint32_t vulnerabilities;   // bitwise OR of VulnerabilityFlag
    
    // Operating thresholds
    struct Thresholds {
        uint8_t max_concurrent_tasks;
        uint16_t deep_work_block_minutes;
        uint16_t light_work_block_minutes;
        uint16_t min_recovery_block_minutes;
        uint8_t max_context_switches_per_hour;
        uint16_t max_social_minutes_per_day;
        float sensory_alert_threshold;     // 0.0 - 1.0
    } thresholds;
    
    // Rhythm slots (24-hour encoded as minute offsets)
    struct TimeSlot {
        uint16_t start_minute;  // 0-1439
        uint16_t end_minute;
    };
    std::array<TimeSlot, 4> deep_work_slots;
    std::array<TimeSlot, 4> light_work_slots;
    std::array<TimeSlot, 4> rest_slots;
    
    uint8_t profile_id;  // for tracking which template this came from
    float confidence;    // 0.0 - 1.0, how well does live data match this profile
} __attribute__((packed));

static_assert(sizeof(NeuroProfile) < 256, "Profile must fit in cache line");

// Compile-time profile templates
namespace profiles {

constexpr NeuroProfile SYSTEMS_HYPERFOCUS = {
    .need_for_structure = TraitLevel::HIGH,
    .novelty_seeking = TraitLevel::LOW,
    .hyperfocus_inclination = TraitLevel::VERY_HIGH,
    .sensory_sensitivity_level = TraitLevel::HIGH,
    .social_energy_capacity = TraitLevel::LOW,
    .executive_function_difficulty = TraitLevel::MEDIUM,
    
    .strengths = static_cast<uint32_t>(StrengthFlag::DEEP_SYSTEMS_THINKING) |
                 static_cast<uint32_t>(StrengthFlag::PATTERN_DETECTION),
    
    .vulnerabilities = static_cast<uint32_t>(VulnerabilityFlag::CONTEXT_SWITCH_OVERLOAD) |
                       static_cast<uint32_t>(VulnerabilityFlag::SENSORY_OVERLOAD) |
                       static_cast<uint32_t>(VulnerabilityFlag::SOCIAL_BURNOUT),
    
    .thresholds = {
        .max_concurrent_tasks = 1,
        .deep_work_block_minutes = 90,
        .light_work_block_minutes = 15,
        .min_recovery_block_minutes = 15,
        .max_context_switches_per_hour = 2,
        .max_social_minutes_per_day = 60,
        .sensory_alert_threshold = 0.5f
    },
    
    .deep_work_slots = {{
        {540, 660},   // 09:00-11:00
        {840, 960},   // 14:00-16:00
        {0, 0}, {0, 0}
    }},
    .light_work_slots = {{
        {660, 720},   // 11:00-12:00
        {960, 1020},  // 16:00-17:00
        {0, 0}, {0, 0}
    }},
    .rest_slots = {{
        {720, 780},   // 12:00-13:00
        {1020, 1440}, // after 17:00
        {0, 0}, {0, 0}
    }},
    
    .profile_id = 1,
    .confidence = 0.0f
};

constexpr NeuroProfile DIVERGENT_CREATIVE = {
    .need_for_structure = TraitLevel::MEDIUM,
    .novelty_seeking = TraitLevel::VERY_HIGH,
    .hyperfocus_inclination = TraitLevel::HIGH,
    .sensory_sensitivity_level = TraitLevel::MEDIUM,
    .social_energy_capacity = TraitLevel::MEDIUM,
    .executive_function_difficulty = TraitLevel::HIGH,
    
    .strengths = static_cast<uint32_t>(StrengthFlag::CREATIVE_DIVERGENT) |
                 static_cast<uint32_t>(StrengthFlag::FAST_CONTEXT_SCANNING),
    
    .vulnerabilities = static_cast<uint32_t>(VulnerabilityFlag::INITIATION_DIFFICULTY) |
                       static_cast<uint32_t>(VulnerabilityFlag::COMPLETION_DIFFICULTY) |
                       static_cast<uint32_t>(VulnerabilityFlag::TIME_BLINDNESS),
    
    .thresholds = {
        .max_concurrent_tasks = 2,
        .deep_work_block_minutes = 45,
        .light_work_block_minutes = 25,
        .min_recovery_block_minutes = 10,
        .max_context_switches_per_hour = 4,
        .max_social_minutes_per_day = 120,
        .sensory_alert_threshold = 0.6f
    },
    
    .deep_work_slots = {{
        {600, 720},   // 10:00-12:00
        {900, 1020},  // 15:00-17:00
        {0, 0}, {0, 0}
    }},
    .light_work_slots = {{
        {510, 600},   // 08:30-10:00
        {780, 900},   // 13:00-15:00
        {0, 0}, {0, 0}
    }},
    .rest_slots = {{
        {720, 780},   // 12:00-13:00
        {1020, 1440}, // after 17:00
        {0, 0}, {0, 0}
    }},
    
    .profile_id = 2,
    .confidence = 0.0f
};

constexpr NeuroProfile SENSORY_SOCIAL_FRAGILE = {
    .need_for_structure = TraitLevel::HIGH,
    .novelty_seeking = TraitLevel::LOW,
    .hyperfocus_inclination = TraitLevel::MEDIUM,
    .sensory_sensitivity_level = TraitLevel::VERY_HIGH,
    .social_energy_capacity = TraitLevel::VERY_LOW,
    .executive_function_difficulty = TraitLevel::MEDIUM,
    
    .strengths = static_cast<uint32_t>(StrengthFlag::EMPATHY_INTEROCEPTION) |
                 static_cast<uint32_t>(StrengthFlag::HIGH_FIDELITY_WORK),
    
    .vulnerabilities = static_cast<uint32_t>(VulnerabilityFlag::SENSORY_OVERLOAD) |
                       static_cast<uint32_t>(VulnerabilityFlag::SOCIAL_BURNOUT) |
                       static_cast<uint32_t>(VulnerabilityFlag::SHUTDOWN_RISK),
    
    .thresholds = {
        .max_concurrent_tasks = 2,
        .deep_work_block_minutes = 60,
        .light_work_block_minutes = 20,
        .min_recovery_block_minutes = 20,
        .max_context_switches_per_hour = 3,
        .max_social_minutes_per_day = 30,
        .sensory_alert_threshold = 0.4f
    },
    
    .deep_work_slots = {{
        {510, 660},   // 08:30-11:00
        {810, 900},   // 13:30-15:00
        {0, 0}, {0, 0}
    }},
    .light_work_slots = {{
        {660, 720},   // 11:00-12:00
        {900, 960},   // 15:00-16:00
        {0, 0}, {0, 0}
    }},
    .rest_slots = {{
        {720, 810},   // 12:00-13:30
        {960, 1440},  // after 16:00
        {0, 0}, {0, 0}
    }},
    
    .profile_id = 3,
    .confidence = 0.0f
};

constexpr NeuroProfile INTENSE_MOOD_VARIANCE = {
    .need_for_structure = TraitLevel::MEDIUM,
    .novelty_seeking = TraitLevel::HIGH,
    .hyperfocus_inclination = TraitLevel::HIGH,
    .sensory_sensitivity_level = TraitLevel::MEDIUM,
    .social_energy_capacity = TraitLevel::HIGH,
    .executive_function_difficulty = TraitLevel::MEDIUM,
    
    .strengths = static_cast<uint32_t>(StrengthFlag::VISION_SETTING) |
                 static_cast<uint32_t>(StrengthFlag::PERSUASIVE_COMMUNICATION),
    
    .vulnerabilities = static_cast<uint32_t>(VulnerabilityFlag::BURNOUT_CYCLES) |
                       static_cast<uint32_t>(VulnerabilityFlag::MOOD_VARIABILITY),
    
    .thresholds = {
        .max_concurrent_tasks = 4,
        .deep_work_block_minutes = 60,
        .light_work_block_minutes = 30,
        .min_recovery_block_minutes = 20,
        .max_context_switches_per_hour = 5,
        .max_social_minutes_per_day = 180,
        .sensory_alert_threshold = 0.65f
    },
    
    .deep_work_slots = {{
        {540, 660},   // 09:00-11:00
        {840, 960},   // 14:00-16:00
        {0, 0}, {0, 0}
    }},
    .light_work_slots = {{
        {660, 720},   // 11:00-12:00
        {960, 1020},  // 16:00-17:00
        {0, 0}, {0, 0}
    }},
    .rest_slots = {{
        {720, 780},   // 12:00-13:00
        {0, 0}, {0, 0}, {0, 0}
    }},
    
    .profile_id = 4,
    .confidence = 0.0f
};

// Array of all profile templates for matcher
constexpr std::array<NeuroProfile, 4> ALL_PROFILES = {
    SYSTEMS_HYPERFOCUS,
    DIVERGENT_CREATIVE,
    SENSORY_SOCIAL_FRAGILE,
    INTENSE_MOOD_VARIANCE
};

} // namespace profiles

} // namespace nnos
