// profile_matcher.hpp
#pragma once
#include "neuro_profile.hpp"
#include <cmath>

namespace nnos {

class ProfileMatcher {
public:
    // Feature weights for similarity computation
    struct FeatureWeights {
        float need_for_structure = 1.0f;
        float novelty_seeking = 1.0f;
        float hyperfocus_inclination = 1.5f;
        float sensory_sensitivity_level = 1.5f;
        float social_energy_capacity = 1.0f;
        float executive_function_difficulty = 1.5f;
    };
    
    constexpr ProfileMatcher(FeatureWeights weights = {}) 
        : weights_(weights) {}
    
    // Find best matching profile from observed behavior
    // Returns profile with highest similarity and updates confidence
    NeuroProfile match_profile(const NeuroProfile& observed) const noexcept {
        float best_similarity = 0.0f;
        NeuroProfile best_profile = profiles::ALL_PROFILES[0];
        
        for (const auto& candidate : profiles::ALL_PROFILES) {
            float sim = compute_similarity(observed, candidate);
            if (sim > best_similarity) {
                best_similarity = sim;
                best_profile = candidate;
            }
        }
        
        best_profile.confidence = best_similarity;
        return best_profile;
    }
    
private:
    FeatureWeights weights_;
    
    // Compute weighted similarity between two profiles
    float compute_similarity(const NeuroProfile& a, 
                            const NeuroProfile& b) const noexcept {
        float total_weight = 0.0f;
        float weighted_sum = 0.0f;
        
        // Trait similarities
        weighted_sum += weights_.need_for_structure * 
                       trait_similarity(a.need_for_structure, b.need_for_structure);
        total_weight += weights_.need_for_structure;
        
        weighted_sum += weights_.novelty_seeking * 
                       trait_similarity(a.novelty_seeking, b.novelty_seeking);
        total_weight += weights_.novelty_seeking;
        
        weighted_sum += weights_.hyperfocus_inclination * 
                       trait_similarity(a.hyperfocus_inclination, b.hyperfocus_inclination);
        total_weight += weights_.hyperfocus_inclination;
        
        weighted_sum += weights_.sensory_sensitivity_level * 
                       trait_similarity(a.sensory_sensitivity_level, b.sensory_sensitivity_level);
        total_weight += weights_.sensory_sensitivity_level;
        
        weighted_sum += weights_.social_energy_capacity * 
                       trait_similarity(a.social_energy_capacity, b.social_energy_capacity);
        total_weight += weights_.social_energy_capacity;
        
        weighted_sum += weights_.executive_function_difficulty * 
                       trait_similarity(a.executive_function_difficulty, b.executive_function_difficulty);
        total_weight += weights_.executive_function_difficulty;
        
        // Bitflag overlaps (Jaccard similarity)
        float strength_overlap = jaccard_similarity(a.strengths, b.strengths);
        float vuln_overlap = jaccard_similarity(a.vulnerabilities, b.vulnerabilities);
        
        weighted_sum += 2.0f * strength_overlap;  // weight of 2.0
        weighted_sum += 2.0f * vuln_overlap;
        total_weight += 4.0f;
        
        return weighted_sum / total_weight;
    }
    
    // Similarity between two trait levels (0.0 = opposite, 1.0 = identical)
    static constexpr float trait_similarity(TraitLevel a, TraitLevel b) noexcept {
        int diff = std::abs(static_cast<int>(a) - static_cast<int>(b));
        return 1.0f - (diff / 4.0f);  // max diff is 4
    }
    
    // Jaccard similarity for bitflags
    static float jaccard_similarity(uint32_t a, uint32_t b) noexcept {
        uint32_t intersection = __builtin_popcount(a & b);
        uint32_t union_bits = __builtin_popcount(a | b);
        return union_bits > 0 ? static_cast<float>(intersection) / union_bits : 0.0f;
    }
};

} // namespace nnos
