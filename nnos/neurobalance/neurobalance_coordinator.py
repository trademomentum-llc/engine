#!/usr/bin/env python3
"""
NeuroBalance Coordinator — Master Integration Layer for NeuroDiOS

This module elevates the concepts from the original neurobalance-engine.py
into the central health governor for the entire NeuroDiOS system.

It operates on:
- The 5 Validated Denominators (the core physics)
- All Behavioral Health Primitives and Clusters
- The Roller Coaster Framework (as active safety/governor during uplift cycles)

Goal: Allow deliberate productive stress (for insight/uplift) while continuously
offsetting unhealthy fluctuations to maintain healthy lifestyle and long-term
neurological sustainability.

No external dependencies beyond the base engine concepts.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import Enum
from typing import Dict, List, Optional

# Import the minimal footprint type system
from .minimal_types import (
    Intensity8, Delta8, MinimalDenominator, MinimalPhase,
    MinimalOffsetAction, compute_action_footprint
)

# =============================================================================
# Validated Denominators (the physics layer) — now with Compute Footprint awareness
# =============================================================================

class ValidatedDenominator(str, Enum):
    FLUCTUATION_DYNAMICS = "fluctuation_dynamics"
    BUDGET_RESOURCE_ACCOUNTING = "budget_resource_accounting"  # Now includes Compute Footprint sub-budget
    CONTRAST_DIFFERENTIAL = "contrast_differential"
    CONTROLLED_OSCILLATION = "controlled_oscillation"
    ADAPTATION_OFFSET = "adaptation_offset"
    PRIMITIVE_TRACEABILITY = "primitive_traceability"
    ORIGIN_VAULT = "origin_vault"
    DRIFT_DETECTION = "drift_detection"


# =============================================================================
# Roller Coaster Phase Awareness
# =============================================================================

class RollerCoasterPhase(str, Enum):
    COAST = "coast"
    ASCENT = "ascent"          # deliberate stress for uplift
    PEAK = "peak"
    DESCENT = "descent"        # relief
    CRYSTALLIZATION = "crystallization"  # protected insight window


# =============================================================================
# Core Data
# =============================================================================

@dataclass(slots=True)
class DenominatorReading:
    """Uses minimal integer types for hot-path efficiency."""
    denominator: ValidatedDenominator
    value: Intensity8           # 0-255 normalized
    velocity: Delta8            # signed delta
    acceleration: Delta8


@dataclass(slots=True)
class OffsetAction:
    """High-level action for human / logging consumption.
    The hot path should prefer MinimalOffsetAction from minimal_types.
    """
    name: str
    description: str
    denominator_involved: List[ValidatedDenominator]
    intensity: Intensity8
    rationale: str

    def to_minimal(self) -> MinimalOffsetAction:
        """Convert to the absolute minimal runtime representation."""
        return MinimalOffsetAction(
            action_id=hash(self.name) & 0xFF,  # cheap stable ID for hot path
            intensity=self.intensity,
            denominator=MinimalDenominator[self.denominator_involved[0].name] if self.denominator_involved else MinimalDenominator.BUDGET,
            phase=MinimalPhase.COAST,  # caller should override with real phase
        )


@dataclass
class UpliftOpportunity:
    """Represents a concrete opportunity for the user to generate real solutions
    or advance a compounding improvement."""
    type: str                    # "solution_fragment" or "compounding_improvement"
    description: str
    denominator_leverage: List[ValidatedDenominator]
    compounding_potential: str


# =============================================================================
# NeuroBalance Coordinator
# =============================================================================

class NeuroBalanceCoordinator:
    """
    The master engine that incorporates the original NeuroBalance logic
    and extends it across the full NeuroDiOS architecture.
    """

    def __init__(self, operating_plane: str = "ANALYSIS"):
        self.operating_plane = operating_plane
        self.current_phase: RollerCoasterPhase = RollerCoasterPhase.COAST
        self.denominator_readings: Dict[ValidatedDenominator, DenominatorReading] = {}
        self.active_offsets: List[OffsetAction] = []

    def update_denominators(self, readings: Dict[ValidatedDenominator, DenominatorReading]) -> None:
        """Called continuously from signal ingestion layer."""
        self.denominator_readings = readings

    def set_roller_coaster_phase(self, phase: RollerCoasterPhase) -> None:
        """The Roller Coaster Framework tells the balance layer where we are."""
        self.current_phase = phase

    def assess_and_offset(self) -> List[OffsetAction]:
        """
        Core loop.
        Returns the set of active offset actions needed right now to keep the user healthy.
        """
        actions: List[OffsetAction] = []

        # === Core Safety Logic using Validated Denominators ===

        # 1. Fluctuation Dynamics check (highest priority) — using minimal int types
        fd = self.denominator_readings.get(ValidatedDenominator.FLUCTUATION_DYNAMICS)
        if fd and fd.acceleration > 45:  # scaled from 0.35 → ~90 on 0-255 range
            actions.append(OffsetAction(
                name="dampen_fluctuation_acceleration",
                description="Reduce rate of state change using minimal compute path.",
                denominator_involved=[ValidatedDenominator.FLUCTUATION_DYNAMICS],
                intensity=Intensity8(min(255, fd.acceleration)),
                rationale="Unhealthy acceleration — using INT8 path only."
            ))

        # 2. Budget Accounting (now includes explicit Compute Footprint sub-budget)
        budget = self.denominator_readings.get(ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING)
        if budget and budget.value < 90:  # scaled
            actions.append(OffsetAction(
                name="enforce_resource_protection",
                description="Protect remaining capacity. Prefer INT16/INT8 actions.",
                denominator_involved=[ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING],
                intensity=Intensity8(255 - budget.value),
                rationale="Budget low — forcing minimal-footprint execution paths."
            ))

        # 3. Roller Coaster specific governor logic
        if self.current_phase == RollerCoasterPhase.ASCENT:
            # Allow some tension, but never let any denominator go critical
            if budget and budget.value < 0.45:
                actions.append(OffsetAction(
                    name="early_descent_trigger",
                    description="Force transition to DESCENT phase early to protect health.",
                    denominator_involved=[ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING, ValidatedDenominator.CONTROLLED_OSCILLATION],
                    intensity=0.8,
                    rationale="Ascent is consuming too much budget. Uplift not worth the damage."
                ))

        if self.current_phase == RollerCoasterPhase.CRYSTALLIZATION:
            # Protect the insight window aggressively
            actions.append(OffsetAction(
                name="protect_crystallization_window",
                description="Minimize all external demand and sensory input for 20-45 minutes.",
                denominator_involved=[ValidatedDenominator.CONTRAST_DIFFERENTIAL, ValidatedDenominator.CONTROLLED_OSCILLATION],
                intensity=0.9,
                rationale="Maximizing uplift from recent contrast while preventing post-cycle crash."
            ))

        # 4. General Adaptation Offset (always available)
        actions.append(OffsetAction(
            name="apply_contextual_adaptation_offset",
            description="Apply the most contextually appropriate offset from the primitive catalogue.",
            denominator_involved=[ValidatedDenominator.ADAPTATION_OFFSET],
            intensity=0.6,
            rationale="Baseline healthy maintenance during system engagement."
        ))

        self.active_offsets = actions
        return actions

    def get_health_summary(self) -> Dict:
        return {
            "current_phase": self.current_phase.value,
            "denominator_status": {k.value: round(v.value, 3) for k, v in self.denominator_readings.items()},
            "active_offsets": [a.name for a in self.active_offsets],
            "operating_plane": self.operating_plane,
        }

    # =====================================================================
    # NEW: Solution-Generating Uplift Layer (per expanded definition)
    # =====================================================================

    def generate_uplift_opportunities(self) -> List[UpliftOpportunity]:
        """
        Actively assists the user in generating solutions to real problems
        and creating compounding capacity for constant improvement.

        Only activates when denominator readings indicate sufficient healthy
        capacity (especially during Crystallization phase or protected windows).
        """
        opportunities: List[UpliftOpportunity] = []

        budget = self.denominator_readings.get(ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING)
        contrast = self.denominator_readings.get(ValidatedDenominator.CONTRAST_DIFFERENTIAL)

        # Guard: Only generate uplift opportunities when the user has real capacity
        if not budget or budget.value < 0.45:
            return opportunities

        if self.current_phase in (RollerCoasterPhase.CRYSTALLIZATION, RollerCoasterPhase.COAST):
            # Use recent contrast + state memory to surface real problems
            opportunities.append(UpliftOpportunity(
                type="solution_fragment",
                description="Recent contrast (stress → relief) has created mental space. Surface one stuck real-world problem and generate 2-3 concrete next micro-actions.",
                denominator_leverage=[ValidatedDenominator.CONTRAST_DIFFERENTIAL, ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING],
                compounding_potential="High if the micro-action is executed and logged within 48 hours."
            ))

            if contrast and contrast.value > 0.6:
                opportunities.append(UpliftOpportunity(
                    type="compounding_improvement",
                    description="Identify one small improvement from a previous cycle that can now be extended or automated. Protect 15-25 minutes to advance it.",
                    denominator_leverage=[ValidatedDenominator.CONTROLLED_OSCILLATION, ValidatedDenominator.ADAPTATION_OFFSET],
                    compounding_potential="Direct compounding of prior uplift into tangible progress."
                ))

        return opportunities


# =============================================================================
# Specific Offset Engines (per major condition/cluster)
# =============================================================================

class ExecutiveDepletionOffsetEngine:
    """Dedicated offset logic for BH-01 style depletion, grounded in the denominators."""

    def recommend(self, readings: Dict[ValidatedDenominator, DenominatorReading]) -> List[OffsetAction]:
        actions = []
        budget = readings.get(ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING)
        if budget and budget.value < 0.4:
            actions.append(OffsetAction(
                name="executive_depletion_micro_recovery",
                description="Enforce 8-12 minute protected low-demand period + single easy closure task.",
                denominator_involved=[ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING, ValidatedDenominator.ADAPTATION_OFFSET],
                intensity=0.85,
                rationale="Direct offset for executive depletion using budget and adaptation primitives."
            ))
        return actions


class MaskingLoadOffsetEngine:
    """Handles Masking + RSD cluster using contrast and oscillation."""

    def recommend(self, readings: Dict[ValidatedDenominator, DenominatorReading]) -> List[OffsetAction]:
        actions = []
        contrast = readings.get(ValidatedDenominator.CONTRAST_DIFFERENTIAL)
        if contrast and contrast.value > 0.7:  # high masking load relative to relief
            actions.append(OffsetAction(
                name="masking_relief_contrast_amplification",
                description="Create deliberate low-demand, low-masking decompression block.",
                denominator_involved=[ValidatedDenominator.CONTRAST_DIFFERENTIAL, ValidatedDenominator.CONTROLLED_OSCILLATION],
                intensity=0.75,
                rationale="Using contrast differential to generate recovery after masking accumulation."
            ))
        return actions


# Example usage pattern the system would run continuously
if __name__ == "__main__":
    coordinator = NeuroBalanceCoordinator(operating_plane="EXECUTE")

    # Simulated live readings from the broader NeuroDiOS signal layer
    readings = {
        ValidatedDenominator.FLUCTUATION_DYNAMICS: DenominatorReading(ValidatedDenominator.FLUCTUATION_DYNAMICS, 0.72, 0.41, 0.29),
        ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING: DenominatorReading(ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING, 0.38, -0.22, -0.15),
        ValidatedDenominator.CONTRAST_DIFFERENTIAL: DenominatorReading(ValidatedDenominator.CONTRAST_DIFFERENTIAL, 0.65, 0.18, 0.09),
        ValidatedDenominator.CONTROLLED_OSCILLATION: DenominatorReading(ValidatedDenominator.CONTROLLED_OSCILLATION, 0.55, 0.12, 0.04),
        ValidatedDenominator.ADAPTATION_OFFSET: DenominatorReading(ValidatedDenominator.ADAPTATION_OFFSET, 0.60, 0.05, 0.02),
    }

    coordinator.update_denominators(readings)
    coordinator.set_roller_coaster_phase(RollerCoasterPhase.ASCENT)

    active = coordinator.assess_and_offset()
    print("Active Offsets:", [a.name for a in active])
    print("Health Summary:", coordinator.get_health_summary())