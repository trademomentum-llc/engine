// communication_bridge.hpp
#pragma once
#include "neuro_profile.hpp"
#include <cstdint>

namespace nnos {

struct IncomingRequest {
    uint32_t id;
    uint8_t importance;       // 0-10
    uint16_t est_effort_min;  // estimated effort
    uint16_t deadline_hours;  // hours from now
    uint8_t social_intensity; // 0-10
} __attribute__((packed));

struct CapacitySnapshot {
    uint8_t energy;           // 0-10
    uint8_t sensory_load;     // 0-10
    uint8_t social_battery;   // 0-10
    uint8_t existing_commitment_load; // 0-10
} __attribute__((packed));

enum class CommDecisionType : uint8_t {
    ACCEPT = 0,
    NEGOTIATE_SCOPE = 1,
    NEGOTIATE_DEADLINE = 2,
    DECLINE = 3
};

struct CommDecision {
    CommDecisionType type;
    uint8_t confidence;       // 0-100
    // concrete numeric adjustments:
    uint16_t new_est_effort_min;
    uint16_t new_deadline_hours;
    bool suggest_async;
    bool require_decompression_after;
} __attribute__((packed));

class CommunicationBridge {
public:
    explicit CommunicationBridge(const NeuroProfile& profile)
        : profile_(profile) {}

    CommDecision evaluate(const IncomingRequest& req,
                          const CapacitySnapshot& cap) const noexcept {
        uint16_t capacity_score = compute_capacity_score(cap);
        uint16_t demand_score = compute_demand_score(req);

        CommDecision decision{};
        decision.new_est_effort_min = req.est_effort_min;
        decision.new_deadline_hours = req.deadline_hours;
        decision.suggest_async = false;
        decision.require_decompression_after = false;

        // Hard guard: if capacity too low relative to demand
        if (capacity_score < 30 && demand_score > 60) {
            decision.type = CommDecisionType::DECLINE;
            decision.confidence = 95;
            return decision;
        }

        // If social battery is low and request is socially intense
        if (cap.social_battery < 3 && req.social_intensity > 6 &&
            has_vulnerability(VulnerabilityFlag::SOCIAL_BURNOUT)) {
            decision.type = CommDecisionType::NEGOTIATE_SCOPE;
            decision.new_est_effort_min = req.est_effort_min / 2;
            decision.suggest_async = true;
            decision.require_decompression_after = true;
            decision.confidence = 90;
            return decision;
        }

        // If time pressure is high vs energy, negotiate deadline
        if (req.deadline_hours < 4 && cap.energy < 5) {
            decision.type = CommDecisionType::NEGOTIATE_DEADLINE;
            decision.new_deadline_hours = static_cast<uint16_t>(req.deadline_hours * 3 / 2);
            decision.confidence = 85;
            return decision;
        }

        // Default: accept with decompression if high social intensity
        decision.type = CommDecisionType::ACCEPT;
        decision.confidence = 80;
        if (req.social_intensity > 5 &&
            has_vulnerability(VulnerabilityFlag::SOCIAL_BURNOUT)) {
            decision.require_decompression_after = true;
        }
        return decision;
    }

    // Anti-masking check: compare intended commitment vs estimated capacity
    bool is_likely_masking(const IncomingRequest& req,
                           const CapacitySnapshot& cap) const noexcept {
        uint16_t capacity_score = compute_capacity_score(cap);
        uint16_t demand_score = compute_demand_score(req);

        // Heuristic: agreeing to high-demand task with low capacity
        if (capacity_score < 40 && demand_score > 70) {
            return true;
        }
        return false;
    }

private:
    const NeuroProfile& profile_;

    bool has_vulnerability(VulnerabilityFlag v) const noexcept {
        return (profile_.vulnerabilities & static_cast<uint32_t>(v)) != 0;
    }

    uint16_t compute_capacity_score(const CapacitySnapshot& cap) const noexcept {
        // All inputs are 0-10. Scale to 0-100.
        uint16_t energy = cap.energy * 10;          // 0-100
        uint16_t sensory = (10 - cap.sensory_load) * 10; // 0-100
        uint16_t social = cap.social_battery * 10;   // 0-100
        uint16_t commitments = (10 - cap.existing_commitment_load) * 10; // 0-100

        uint16_t score = (energy * 40 + sensory * 30 + social * 20 + commitments * 10) / 100;
        return score;
    }

    uint16_t compute_demand_score(const IncomingRequest& req) const noexcept {
        // effort: 4h = 1.0. In 0-100 scale: min(est_effort_min * 100 / 240, 100)
        uint16_t effort = req.est_effort_min * 100 / 240;
        if (effort > 100) effort = 100;

        uint16_t importance = req.importance * 10; // 0-100
        uint16_t social = req.social_intensity * 10; // 0-100

        uint16_t score = (effort * 50 + importance * 30 + social * 20) / 100;
        return score;
    }
};

} // namespace nnos
