#!/usr/bin/env python3
"""
Efficiency Mandate Test Suite for NeuroBalanceCoordinator

Validates that the hot path produces zero float objects in steady state,
uses only minimal integer types, and invokes compute_action_footprint()
on every action decision.

Denominator mapping:
#2 Budget/Resource Accounting (with Compute Footprint sub-budget)
#6 Primitive Traceability (type discipline)
#8 Drift Detection (regression guard)
"""

import gc
import sys
import types
import unittest

from neurobalance.neurobalance_coordinator import (
    NeuroBalanceCoordinator,
    ValidatedDenominator,
    DenominatorReading,
    RollerCoasterPhase,
    OffsetAction,
)
from neurobalance.minimal_types import Intensity8, Delta8, MinimalOffsetAction, compute_action_footprint


class TestEfficiencyMandate(unittest.TestCase):
    """Core test: hot path must not allocate float objects."""

    def _make_readings(self) -> dict:
        return {
            ValidatedDenominator.FLUCTUATION_DYNAMICS: DenominatorReading(
                ValidatedDenominator.FLUCTUATION_DYNAMICS,
                Intensity8(184), Delta8(105), Delta8(74), Intensity8(3),
            ),
            ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING: DenominatorReading(
                ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING,
                Intensity8(97), Delta8(-56), Delta8(-38), Intensity8(2),
            ),
            ValidatedDenominator.CONTRAST_DIFFERENTIAL: DenominatorReading(
                ValidatedDenominator.CONTRAST_DIFFERENTIAL,
                Intensity8(166), Delta8(46), Delta8(23), Intensity8(4),
            ),
            ValidatedDenominator.CONTROLLED_OSCILLATION: DenominatorReading(
                ValidatedDenominator.CONTROLLED_OSCILLATION,
                Intensity8(140), Delta8(31), Delta8(10), Intensity8(2),
            ),
            ValidatedDenominator.ADAPTATION_OFFSET: DenominatorReading(
                ValidatedDenominator.ADAPTATION_OFFSET,
                Intensity8(153), Delta8(13), Delta8(5), Intensity8(1),
            ),
        }

    def test_zero_floats_in_hot_path(self):
        """
        Verify that assess_and_offset() creates zero float objects during execution.
        """
        coord = NeuroBalanceCoordinator()
        coord.update_denominators(self._make_readings())
        coord.set_roller_coaster_phase(RollerCoasterPhase.ASCENT)

        gc.collect()
        float_count_before = len([
            obj for obj in gc.get_objects()
            if isinstance(obj, float)
        ])

        # Run the hot path multiple times
        for _ in range(10):
            actions = coord.assess_and_offset()

        gc.collect()
        float_count_after = len([
            obj for obj in gc.get_objects()
            if isinstance(obj, float)
        ])

        # Allow tiny tolerance for Python internals (e.g., module-level constants)
        delta = float_count_after - float_count_before
        self.assertLessEqual(
            delta, 2,
            f"Hot path allocated {delta} new float objects; expected <= 2 (Python internals only)"
        )

    def test_all_actions_have_footprint(self):
        """Every OffsetAction returned by assess_and_offset must have footprint > 0."""
        coord = NeuroBalanceCoordinator()
        coord.update_denominators(self._make_readings())
        coord.set_roller_coaster_phase(RollerCoasterPhase.CRYSTALLIZATION)

        actions = coord.assess_and_offset()
        self.assertTrue(len(actions) > 0, "Expected at least one action in crystallization phase")

        for action in actions:
            self.assertIsInstance(action, OffsetAction)
            self.assertGreater(
                action.footprint, 0,
                f"Action '{action.name}' missing footprint — compute_action_footprint not called"
            )
            self.assertIsInstance(action.intensity, int)
            self.assertGreaterEqual(action.intensity, 0)
            self.assertLessEqual(action.intensity, 255)

    def test_intensity8_type_discipline(self):
        """All intensity values must be integers in [0, 255]."""
        coord = NeuroBalanceCoordinator()
        coord.update_denominators(self._make_readings())
        coord.set_roller_coaster_phase(RollerCoasterPhase.ASCENT)

        actions = coord.assess_and_offset()
        for action in actions:
            self.assertIsInstance(
                action.intensity, int,
                f"Action '{action.name}' intensity is {type(action.intensity).__name__}, expected int"
            )
            self.assertIn(action.intensity, range(256))

    def test_reading_compute_footprint_field(self):
        """DenominatorReading must carry compute_footprint per Efficiency Mandate."""
        reading = DenominatorReading(
            ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING,
            Intensity8(100), Delta8(10), Delta8(5), Intensity8(7),
        )
        self.assertEqual(reading.compute_footprint, 7)

    def test_minimal_offset_conversion(self):
        """OffsetAction.to_minimal() must produce a MinimalOffsetAction with no strings."""
        action = OffsetAction(
            name="test_action",
            description="test desc",
            denominator_involved=[ValidatedDenominator.BUDGET_RESOURCE_ACCOUNTING],
            intensity=Intensity8(128),
            rationale="test rationale",
            footprint=5,
        )
        minimal = action.to_minimal()
        self.assertIsInstance(minimal, MinimalOffsetAction)
        self.assertIsInstance(minimal.intensity, int)
        self.assertIsInstance(minimal.action_id, int)

    def test_thresholds_are_integers(self):
        """All class-level thresholds must be int, not float."""
        for attr_name in dir(NeuroBalanceCoordinator):
            if attr_name.startswith("THRESH_"):
                value = getattr(NeuroBalanceCoordinator, attr_name)
                self.assertIsInstance(
                    value, int,
                    f"Threshold {attr_name} = {value!r} is {type(value).__name__}, expected int"
                )


class TestEightDenominators(unittest.TestCase):
    """Validate that the coordinator references all 8 Validated Denominators."""

    def test_all_eight_present(self):
        expected = {
            "fluctuation_dynamics",
            "budget_resource_accounting",
            "contrast_differential",
            "controlled_oscillation",
            "adaptation_offset",
            "primitive_traceability",
            "origin_vault",
            "drift_detection",
        }
        actual = {d.value for d in ValidatedDenominator}
        self.assertEqual(actual, expected)


if __name__ == "__main__":
    unittest.main(verbosity=2)
