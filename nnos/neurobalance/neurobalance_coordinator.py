#!/usr/bin/env python3
"""
NeuroBalance Coordinator — Master Integration Layer for NeuroDiOS

This module elevates the concepts from the original neurobalance-engine.py
into the central health governor for the entire NeuroDiOS system.

It operates on:
- The 8 Validated Denominators (the core physics)
- All Behavioral Health Primitives and Clusters
- The Roller Coaster Framework (as active safety/governor during uplift cycles)
- The Efficiency Mandate (minimal integer types, compute footprint awareness)

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
    Intensity8, Delta8, FixedQ7_9,
    MinimalDenominator, MinimalPhase,
    MinimalOffsetAction, compute_action_footprint,
    to_q7_9, from_q7_9,
)

# Mapping from high-level ValidatedDenominator names to compact MinimalDenominator
_DENOMINATOR_MAP: dict[str, MinimalDenominator] = {
    "FLUCTUATION_DYNAMICS": MinimalDenominator.FLUCTUATION,
    "BUDGET_RESOURCE_ACCOUNTING": MinimalDenominator.BUDGET,
    "CONTRAST_DIFFERENTIAL": MinimalDenominator.CONTRAST,
    "CONTROLLED_OSCILLATION": MinimalDenominator.OSCILLATION,
    "ADAPTATION_OFFSET": MinimalDenominator.OFFSET,
    "PRIMITIVE_TRACEABILITY": MinimalDenominator.TRACEABILITY,
    "ORIGIN_VAULT": MinimalDenominator.ORIGIN_VAULT,
    "DRIFT_DETECTION": MinimalDenominator.DRIFT,
}

# =============================================================================
# Validated Denominators (the physics layer) — 8 canonical + Compute Footprint
# =============================================================================

class ValidatedDenominator(str, Enum):
    FLUCTUATION_DYNAMICS = "fluctuation_dynamics"
    BUDGET_RESOURCE_ACCOUNTING = "budget_resource_accounting"
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
# Core Data — all hot-path types use minimal integer representations
# =============================================================================

@dataclass(slots=True)
class DenominatorReading:
    """Uses minimal integer types for hot-path efficiency.

    All normalized values are scaled to [0, 255] (Intensity8) or [-128, 127] (Delta8).
    No floats enter the hot path.
    """
    denominator: ValidatedDenominator
    value: Intensity8           # 0-255 normalized
    velocity: Delta8            # signed delta
    acceleration: Delta8
    compute_footprint: Intensity8 = Intensity8(0)  # sub-budget per Efficiency Mandate


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
    footprint: int = 0          # populated by compute_action_footprint at creation

    def to_minimal(self) -> MinimalOffsetAction:
        """Convert to the absolute minimal runtime representation."""
        return MinimalOffsetAction(
            action_id=hash(self.name) & 0xFF,
            intensity=self.intensity,
            denominator=_DENOMINATOR_MAP[self.denominator_involved[0].name] if self.denominator_involved else MinimalDenominator.BUDGET,
            phase=MinimalPhase.COAST,
            _rationale_id=None,
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

    Efficiency Mandate compliance:
    - Hot path (assess_and_offset) uses only integer types.
    - compute_action_footprint() is invoked on every action decision.
    - No float objects are created during steady-state operation.
    """

    # Pre-computed thresholds on the 0-255 Intensity8 scale to avoid float math
    THRESH_BUDGET_LOW = Intensity8(115)          # ~0.45 * 255
    THRESH_BUDGET_CRITICAL = Intensity8(102)     # ~0.40 * 255
    THRESH_BUDGET_GUARD = Intensity8(90)         # ~0.35 * 255
    THRESH_CONTRAST_HIGH = Intensity8(179)       # ~0.70 * 255
    THRESH_CONTRAST_VERY_HIGH = Intensity8(191)  # ~0.75 * 255
    THRESH_INTENSITY_STRONG = Intensity8(204)    # ~0.80 * 255
    THRESH_INTENSITY_MAX = Intensity8(230)       # ~0.90 * 255
    THRESH_INTENSITY_MODERATE = Intensity8(153)  # ~0.60 * 255
    THRESH_ACCEL_DAMPEN = Delta8(45)             # raw delta threshold

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

    def _make_offset(
        self,
        name: str,
        description: str,
        denominators: List[ValidatedDenominator],
        intensity: Intensity8,
        rationale: str,
    ) -> OffsetAction:
        """Factory that wraps every action with footprint calculation."""
        minimal = MinimalOffsetAction(
            action_id=hash(name) & 0xFF,
            intensity=intensity,
            denominator=_DENOMINATOR_MAP[denominators[0].name] if denominators else MinimalDenominator.BUDGET,
            phase=MinimalPhase[self.current_phase.name],
        )
        footprint = compute_action_footprint(minimal)
        return OffsetAction(
            name=name,
            description=description,
            denominator_involved=denominators,
            intensity=intensity,
            rationale=rationale,
            footprint=footprint,
        )

    def assess_and_offset(self) -> List[OffsetAction]:
        """
        Core loop.
        Returns the set of active offset actions needed right now to keep the user healthy.

        Efficiency Mandate: this method creates zero float objects in steady state.
        All comparisons use integer thresholds; all intensities are Intensity8.
        """
        actions: List[OffsetAction] = []

        # 1. Fluctuation Dynamics check (highest priority)
        fd = self.denominator_readings.get(ValidatedDenominator.FLUCTUATION_DYNAMICS)
        if fd and fd.acceleration > self.THRESH_ACCEL_DAMPEN:
            actions.append(self._make_offset(
                name="dampen_fluctuation_acceleration",
                description="Reduce rate of state change using minimal compute path.",
                denominators=[ValidatedDenominator.FLUCTUATION_DYNAMICS],
                intensity=Intensity8(min(255, fd.acceleration)),
                rationale="Unhealthy acceleration — using INT8 path only.",
            ))

        # 2. Budget Accounting (includes explicit Compute Footprint sub-budget)
        budget = self.denominator_readings.get(ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING)
        if budget and budget.value < self.THRESH_BUDGET_GUARD:
            actions.append(self._make_offset(
                name="enforce_resource_protection",
                description="Protect remaining capacity. Prefer INT16/INT8 actions.",
                denominators=[ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING],
                intensity=Intensity8(255 - budget.value),
                rationale="Budget low — forcing minimal-footprint execution paths.",
            ))

        # 3. Roller Coaster specific governor logic
        if self.current_phase == RollerCoasterPhase.ASCENT:
            if budget and budget.value < self.THRESH_BUDGET_LOW:
                actions.append(self._make_offset(
                    name="early_descent_trigger",
                    description="Force transition to DESCENT phase early to protect health.",
                    denominators=[ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING, ValidatedDenominator.CONTROLLED_OSCILLATION],
                    intensity=self.THRESH_INTENSITY_STRONG,
                    rationale="Ascent is consuming too much budget. Uplift not worth the damage.",
                ))

        if self.current_phase == RollerCoasterPhase.CRYSTALLIZATION:
            actions.append(self._make_offset(
                name="protect_crystallization_window",
                description="Minimize all external demand and sensory input for 20-45 minutes.",
                denominators=[ValidatedDenominator.CONTRAST_DIFFERENTIAL, ValidatedDenominator.CONTROLLED_OSCILLATION],
                intensity=self.THRESH_INTENSITY_MAX,
                rationale="Maximizing uplift from recent contrast while preventing post-cycle crash.",
            ))

        # 4. General Adaptation Offset (always available)
        actions.append(self._make_offset(
            name="apply_contextual_adaptation_offset",
            description="Apply the most contextually appropriate offset from the primitive catalogue.",
            denominators=[ValidatedDenominator.ADAPTATION_OFFSET],
            intensity=self.THRESH_INTENSITY_MODERATE,
            rationale="Baseline healthy maintenance during system engagement.",
        ))

        self.active_offsets = actions
        return actions

    def get_health_summary(self) -> Dict:
        return {
            "current_phase": self.current_phase.value,
            "denominator_status": {k.value: v.value for k, v in self.denominator_readings.items()},
            "active_offsets": [
                {"name": a.name, "intensity": a.intensity, "footprint": a.footprint}
                for a in self.active_offsets
            ],
            "operating_plane": self.operating_plane,
        }

    # =====================================================================
    # Solution-Generating Uplift Layer
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
        if not budget or budget.value < self.THRESH_BUDGET_LOW:
            return opportunities

        if self.current_phase in (RollerCoasterPhase.CRYSTALLIZATION, RollerCoasterPhase.COAST):
            opportunities.append(UpliftOpportunity(
                type="solution_fragment",
                description="Recent contrast (stress → relief) has created mental space. Surface one stuck real-world problem and generate 2-3 concrete next micro-actions.",
                denominator_leverage=[ValidatedDenominator.CONTRAST_DIFFERENTIAL, ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING],
                compounding_potential="High if the micro-action is executed and logged within 48 hours.",
            ))

            if contrast and contrast.value > self.THRESH_INTENSITY_MODERATE:
                opportunities.append(UpliftOpportunity(
                    type="compounding_improvement",
                    description="Identify one small improvement from a previous cycle that can now be extended or automated. Protect 15-25 minutes to advance it.",
                    denominator_leverage=[ValidatedDenominator.CONTROLLED_OSCILLATION, ValidatedDenominator.ADAPTATION_OFFSET],
                    compounding_potential="Direct compounding of prior uplift into tangible progress.",
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
        if budget and budget.value < Intensity8(102):  # ~0.40 * 255
            minimal = MinimalOffsetAction(
                action_id=0x01,
                intensity=Intensity8(217),  # ~0.85 * 255
                denominator=MinimalDenominator.BUDGET,
                phase=MinimalPhase.COAST,
            )
            actions.append(OffsetAction(
                name="executive_depletion_micro_recovery",
                description="Enforce 8-12 minute protected low-demand period + single easy closure task.",
                denominator_involved=[ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING, ValidatedDenominator.ADAPTATION_OFFSET],
                intensity=Intensity8(217),
                rationale="Direct offset for executive depletion using budget and adaptation primitives.",
                footprint=compute_action_footprint(minimal),
            ))
        return actions


class MaskingLoadOffsetEngine:
    """Handles Masking + RSD cluster using contrast and oscillation."""

    def recommend(self, readings: Dict[ValidatedDenominator, DenominatorReading]) -> List[OffsetAction]:
        actions = []
        contrast = readings.get(ValidatedDenominator.CONTRAST_DIFFERENTIAL)
        if contrast and contrast.value > Intensity8(179):  # ~0.70 * 255
            minimal = MinimalOffsetAction(
                action_id=0x02,
                intensity=Intensity8(191),  # ~0.75 * 255
                denominator=MinimalDenominator.CONTRAST,
                phase=MinimalPhase.COAST,
            )
            actions.append(OffsetAction(
                name="masking_relief_contrast_amplification",
                description="Create deliberate low-demand, low-masking decompression block.",
                denominator_involved=[ValidatedDenominator.CONTRAST_DIFFERENTIAL, ValidatedDenominator.CONTROLLED_OSCILLATION],
                intensity=Intensity8(191),
                rationale="Using contrast differential to generate recovery after masking accumulation.",
                footprint=compute_action_footprint(minimal),
            ))
        return actions


# =============================================================================
# Example usage pattern the system would run continuously
# =============================================================================
if __name__ == "__main__":
    coordinator = NeuroBalanceCoordinator(operating_plane="EXECUTE")

    # Simulated live readings from the broader NeuroDiOS signal layer
    # All values use the Intensity8 (0-255) / Delta8 (-128..127) scale
    readings = {
        ValidatedDenominator.FLUCTUATION_DYNAMICS: DenominatorReading(
            ValidatedDenominator.FLUCTUATION_DYNAMICS,
            Intensity8(184),   # ~0.72 * 255
            Delta8(105),       # ~0.41 * 255 scaled to signed
            Delta8(74),        # ~0.29 * 255 scaled to signed
            Intensity8(3),
        ),
        ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING: DenominatorReading(
            ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING,
            Intensity8(97),    # ~0.38 * 255
            Delta8(-56),       # ~-0.22 * 255
            Delta8(-38),       # ~-0.15 * 255
            Intensity8(2),
        ),
        ValidatedDenominator.CONTRAST_DIFFERENTIAL: DenominatorReading(
            ValidatedDenominator.CONTRAST_DIFFERENTIAL,
            Intensity8(166),   # ~0.65 * 255
            Delta8(46),        # ~0.18 * 255
            Delta8(23),        # ~0.09 * 255
            Intensity8(4),
        ),
        ValidatedDenominator.CONTROLLED_OSCILLATION: DenominatorReading(
            ValidatedDenominator.CONTROLLED_OSCILLATION,
            Intensity8(140),   # ~0.55 * 255
            Delta8(31),        # ~0.12 * 255
            Delta8(10),        # ~0.04 * 255
            Intensity8(2),
        ),
        ValidatedDenominator.ADAPTATION_OFFSET: DenominatorReading(
            ValidatedDenominator.ADAPTATION_OFFSET,
            Intensity8(153),   # ~0.60 * 255
            Delta8(13),        # ~0.05 * 255
            Delta8(5),         # ~0.02 * 255
            Intensity8(1),
        ),
    }

    coordinator.update_denominators(readings)
    coordinator.set_roller_coaster_phase(RollerCoasterPhase.ASCENT)

    active = coordinator.assess_and_offset()
    print("Active Offsets:", [(a.name, a.intensity, a.footprint) for a in active])
    print("Health Summary:", coordinator.get_health_summary())
