// profile_matcher.hpp
#pragma once
#include "neuro_profile.hpp"

namespace nnos {

class ProfileMatcher {
public:
    // Feature weights for similarity computation (scaled by 100)
    struct FeatureWeights {
        uint16_t need_for_structure;
        uint16_t novelty_seeking;
        uint16_t hyperfocus_inclination;
        uint16_t sensory_sensitivity_level;
        uint16_t social_energy_capacity;
        uint16_t executive_function_difficulty;
        FeatureWeights()
            : need_for_structure(100), novelty_seeking(100),
              hyperfocus_inclination(150), sensory_sensitivity_level(150),
              social_energy_capacity(100), executive_function_difficulty(150) {}
    };

    ProfileMatcher(FeatureWeights weights = FeatureWeights());

    // Find best matching profile from observed behavior
    // Returns profile with highest similarity and updates confidence (0-255)
    NeuroProfile match_profile(const NeuroProfile& observed) const noexcept {
        uint16_t best_similarity = 0;
        NeuroProfile best_profile = profiles::ALL_PROFILES[0];

        for (const auto& candidate : profiles::ALL_PROFILES) {
            uint16_t sim = compute_similarity(observed, candidate);
            if (sim > best_similarity) {
                best_similarity = sim;
                best_profile = candidate;
            }
        }

        best_profile.confidence = static_cast<uint8_t>((best_similarity * 255) / 100);
        return best_profile;
    }

private:
    FeatureWeights weights_;

    // Compute weighted similarity between two profiles (0-100)
    uint16_t compute_similarity(const NeuroProfile& a,
                                const NeuroProfile& b) const noexcept {
        uint32_t total_weight = 0;
        uint32_t weighted_sum = 0;

        weighted_sum += static_cast<uint32_t>(weights_.need_for_structure) *
                        trait_similarity(a.need_for_structure, b.need_for_structure);
        total_weight += weights_.need_for_structure;

        weighted_sum += static_cast<uint32_t>(weights_.novelty_seeking) *
                        trait_similarity(a.novelty_seeking, b.novelty_seeking);
        total_weight += weights_.novelty_seeking;

        weighted_sum += static_cast<uint32_t>(weights_.hyperfocus_inclination) *
                        trait_similarity(a.hyperfocus_inclination, b.hyperfocus_inclination);
        total_weight += weights_.hyperfocus_inclination;

        weighted_sum += static_cast<uint32_t>(weights_.sensory_sensitivity_level) *
                        trait_similarity(a.sensory_sensitivity_level, b.sensory_sensitivity_level);
        total_weight += weights_.sensory_sensitivity_level;

        weighted_sum += static_cast<uint32_t>(weights_.social_energy_capacity) *
                        trait_similarity(a.social_energy_capacity, b.social_energy_capacity);
        total_weight += weights_.social_energy_capacity;

        weighted_sum += static_cast<uint32_t>(weights_.executive_function_difficulty) *
                        trait_similarity(a.executive_function_difficulty, b.executive_function_difficulty);
        total_weight += weights_.executive_function_difficulty;

        // Bitflag overlaps (Jaccard similarity)
        uint16_t strength_overlap = jaccard_similarity(a.strengths, b.strengths);
        uint16_t vuln_overlap = jaccard_similarity(a.vulnerabilities, b.vulnerabilities);

        weighted_sum += 200 * strength_overlap;  // weight of 2.0 scaled by 100
        weighted_sum += 200 * vuln_overlap;
        total_weight += 400;

        return static_cast<uint16_t>((weighted_sum * 100) / (total_weight * 100));
    }

    // Similarity between two trait levels (0-100; 0 = opposite, 100 = identical)
    static constexpr uint16_t trait_similarity(TraitLevel a, TraitLevel b) noexcept {
        int diff = static_cast<int>(a) > static_cast<int>(b)
                       ? static_cast<int>(a) - static_cast<int>(b)
                       : static_cast<int>(b) - static_cast<int>(a);
        return static_cast<uint16_t>(100 - diff * 25);  // max diff is 4
    }

    // Jaccard similarity for bitflags (0-100)
    static uint16_t jaccard_similarity(uint32_t a, uint32_t b) noexcept {
        uint32_t intersection = __builtin_popcount(a & b);
        uint32_t union_bits = __builtin_popcount(a | b);
        return union_bits > 0
                   ? static_cast<uint16_t>((intersection * 100) / union_bits)
                   : 0;
    }
};

inline ProfileMatcher::ProfileMatcher(FeatureWeights weights)
    : weights_(weights) {}

} // namespace nnos
