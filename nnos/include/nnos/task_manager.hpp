// task_manager.hpp
#pragma once
#include "neuro_profile.hpp"
#include <cstdint>

namespace nnos {

enum class TaskDemand : uint8_t {
    DEEP_SYSTEMS      = 1 << 0,
    CREATIVE_DIVERGENT = 1 << 1,
    ADMIN_EXEC        = 1 << 2,
    SOCIAL_COMM       = 1 << 3,
    SENSORY_RISK      = 1 << 4
};

struct Task {
    uint32_t id;
    uint8_t demand_flags;  // bitwise OR of TaskDemand
    uint16_t est_effort_minutes;
    uint8_t importance;    // 0-10
    uint8_t social_cost;   // 0-10
    uint8_t sensory_cost;  // 0-10
    uint64_t deadline_timestamp;
    
    // Computed during intake
    bool fit_strength;
    bool fit_risk;
    uint16_t scheduled_minute;  // minute of day, 0-1439
} __attribute__((packed));

class TaskManager {
public:
    explicit TaskManager(const NeuroProfile& profile) 
        : profile_(profile), active_task_count_(0) {}
    
    // NNOS_TASK_INTAKE procedure
    bool intake_task(Task& task) noexcept {
        // Step 1: Match demand to profile strengths/vulnerabilities
        task.fit_strength = match_strengths(task.demand_flags);
        task.fit_risk = match_vulnerabilities(task);
        
        // Step 2: Check capacity
        if (active_task_count_ >= profile_.thresholds.max_concurrent_tasks) {
            return false;  // defer
        }
        
        // Step 3: Schedule
        if (task.fit_strength && !task.fit_risk) {
            task.scheduled_minute = find_deep_slot(task.est_effort_minutes);
        } else if (task.fit_risk) {
            task.scheduled_minute = find_supported_slot(task.est_effort_minutes);
        } else {
            task.scheduled_minute = find_light_slot(task.est_effort_minutes);
        }
        
        if (task.scheduled_minute == INVALID_SLOT) {
            return false;
        }
        
        ++active_task_count_;
        return true;
    }
    
    void complete_task() noexcept {
        if (active_task_count_ > 0) --active_task_count_;
    }
    
private:
    static constexpr uint16_t INVALID_SLOT = 0xFFFF;
    
    const NeuroProfile& profile_;
    uint8_t active_task_count_;
    
    bool match_strengths(uint8_t demand_flags) const noexcept {
        // Map task demands to strength flags
        uint32_t required_strengths = 0;
        
        if (demand_flags & static_cast<uint8_t>(TaskDemand::DEEP_SYSTEMS)) {
            required_strengths |= static_cast<uint32_t>(StrengthFlag::DEEP_SYSTEMS_THINKING);
        }
        if (demand_flags & static_cast<uint8_t>(TaskDemand::CREATIVE_DIVERGENT)) {
            required_strengths |= static_cast<uint32_t>(StrengthFlag::CREATIVE_DIVERGENT);
        }
        
        return (profile_.strengths & required_strengths) != 0;
    }
    
    bool match_vulnerabilities(const Task& task) const noexcept {
        // High social/sensory cost triggers vulnerability flags
        bool has_risk = false;
        
        if (task.social_cost > 7 && 
            (profile_.vulnerabilities & static_cast<uint32_t>(VulnerabilityFlag::SOCIAL_BURNOUT))) {
            has_risk = true;
        }
        
        if (task.sensory_cost > 7 && 
            (profile_.vulnerabilities & static_cast<uint32_t>(VulnerabilityFlag::SENSORY_OVERLOAD))) {
            has_risk = true;
        }
        
        if (task.demand_flags & static_cast<uint8_t>(TaskDemand::ADMIN_EXEC) &&
            (profile_.vulnerabilities & static_cast<uint32_t>(VulnerabilityFlag::INITIATION_DIFFICULTY))) {
            has_risk = true;
        }
        
        return has_risk;
    }
    
    uint16_t find_deep_slot(uint16_t duration) const noexcept {
        for (const auto& slot : profile_.deep_work_slots) {
            if (slot.start_minute == 0) break;
            uint16_t slot_duration = slot.end_minute - slot.start_minute;
            if (slot_duration >= duration) {
                return slot.start_minute;
            }
        }
        return INVALID_SLOT;
    }
    
    uint16_t find_light_slot(uint16_t duration) const noexcept {
        for (const auto& slot : profile_.light_work_slots) {
            if (slot.start_minute == 0) break;
            uint16_t slot_duration = slot.end_minute - slot.start_minute;
            if (slot_duration >= duration) {
                return slot.start_minute;
            }
        }
        return INVALID_SLOT;
    }
    
    uint16_t find_supported_slot(uint16_t duration) const noexcept {
        // For high-risk tasks, prefer slots with more buffer
        // Simple heuristic: use light slots with extra time
        for (const auto& slot : profile_.light_work_slots) {
            if (slot.start_minute == 0) break;
            uint16_t slot_duration = slot.end_minute - slot.start_minute;
            if (slot_duration >= duration + 10) {  // +10 min buffer
                return slot.start_minute;
            }
        }
        return INVALID_SLOT;
    }
};

} // namespace nnos
