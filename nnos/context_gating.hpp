// context_gating.hpp
#pragma once
#include "neuro_profile.hpp"
#include <cstdint>

namespace nnos {

struct ContextState {
    uint32_t active_task_id;
    uint8_t active_task_importance;      // 0-10
    uint8_t incoming_task_importance;    // 0-10
    bool incoming_is_interrupt;          // e.g., notification
    bool hyperfocus_active;
    uint32_t hyperfocus_minutes;         // minutes in current focus
    bool physiological_risk_high;        // derived from sensors
    bool self_report_dissociated;        // 0/1 flag
} __attribute__((packed));

enum class ContextDecision : uint8_t {
    KEEP_FOCUS = 0,
    ALLOW_SWITCH_WITH_HANDOFF = 1,
    DEFER_INCOMING = 2,
    BREAK_HYPERFOCUS = 3
};

struct HandoffNote {
    uint32_t task_id;
    uint16_t last_step_index;
    uint16_t next_step_index;
    uint16_t timestamp_minute_of_day;  // for re-entry alignment
} __attribute__((packed));

class ContextGating {
public:
    explicit ContextGating(const NeuroProfile& profile)
        : profile_(profile) {}

    // NNOS_CONTEXT_GATING core procedure
    ContextDecision evaluate(const ContextState& state, 
                             HandoffNote& handoff_out) const noexcept {
        // 1. If hyperfocus is dangerous, break it
        if (state.hyperfocus_active && 
            (state.physiological_risk_high || state.self_report_dissociated)) {
            // Here we’d also prepare a recovery handoff
            handoff_out = make_handoff(state.active_task_id);
            return ContextDecision::BREAK_HYPERFOCUS;
        }

        // 2. If hyperfocus is on a strength, protect it
        if (state.hyperfocus_active && is_hyperfocus_protectable()) {
            // Only allow very high priority interrupts
            if (state.incoming_is_interrupt &&
                state.incoming_task_importance + 2 < state.active_task_importance) {
                return ContextDecision::DEFER_INCOMING;
            }
            // Otherwise keep focus
            return ContextDecision::KEEP_FOCUS;
        }

        // 3. Normal context switch logic
        if (state.incoming_is_interrupt) {
            // Too many switches per hour? Prefer deferral
            if (current_switch_count_ >= profile_.thresholds.max_context_switches_per_hour) {
                return ContextDecision::DEFER_INCOMING;
            }

            if (state.incoming_task_importance > state.active_task_importance) {
                // allow switch, but generate handoff
                handoff_out = make_handoff(state.active_task_id);
                ++current_switch_count_;
                return ContextDecision::ALLOW_SWITCH_WITH_HANDOFF;
            } else {
                return ContextDecision::DEFER_INCOMING;
            }
        }

        // No interrupt, no hyperfocus → keep focus
        return ContextDecision::KEEP_FOCUS;
    }

    void reset_switch_counter() noexcept { current_switch_count_ = 0; }

private:
    const NeuroProfile& profile_;
    mutable uint8_t current_switch_count_ = 0;

    bool is_hyperfocus_protectable() const noexcept {
        // Hyperfocus is treated as an asset if profile supports it
        return profile_.hyperfocus_inclination >= TraitLevel::HIGH;
    }

    HandoffNote make_handoff(uint32_t task_id) const noexcept {
        // In a real system, last/next step indices would be fetched
        // from the task execution context. Here we stub them.
        HandoffNote note{};
        note.task_id = task_id;
        note.last_step_index = 0;
        note.next_step_index = 1;
        note.timestamp_minute_of_day = 0;
        return note;
    }
};

} // namespace nnos
