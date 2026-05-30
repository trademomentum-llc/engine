"""
Minimal Footprint Type System for NeuroDiOS Actions

Objective: Every action, primitive evaluation, and mechanism must use the
smallest deterministic data type that safely satisfies the required precision.

This module defines the canonical minimal types used across the NeuroBalance
Coordinator, Offset Engines, and Roller Coaster governance.

Design rules:
- Prefer integer types (i8, i16, u8, u16) over floating point.
- Use fixed-point scaling (e.g. Q8.8 or Q7.9) when fractional values are needed.
- Reserve FP32/FP16 only for cases where the physics genuinely requires it
  (rare in behavioral control loops).
- All normalized values are in the range [0, 255] or [-128, 127] mapped to the
  actual domain at the call site.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
from typing import NewType

# =============================================================================
# Minimal Integer Types (preferred)
# =============================================================================

# Normalized intensity / capacity / load in [0, 255]
# Maps to 0.0 - 1.0 at the application layer
Intensity8 = NewType("Intensity8", int)          # u8

# Signed velocity / delta in [-128, 127]
# Sufficient for most behavioral change rates
Delta8 = NewType("Delta8", int)

# Higher precision when needed (e.g. accumulated debt)
Intensity16 = NewType("Intensity16", int)        # i16
Delta16 = NewType("Delta16", int)

# =============================================================================
# Fixed-Point Types (when we need fractional behavior without float)
# =============================================================================

# Q7.9 fixed point (range ~ -256 to +256 with ~0.002 precision)
# Excellent for most control and budget calculations
FixedQ7_9 = NewType("FixedQ7_9", int)

def to_q7_9(value: float) -> FixedQ7_9:
    """Convert float in reasonable range to Q7.9"""
    scaled = int(value * 512)  # 2^9
    return FixedQ7_9(max(-32768, min(32767, scaled)))

def from_q7_9(fp: FixedQ7_9) -> float:
    return fp / 512.0

# =============================================================================
# Strongly Typed Enums (zero token / runtime cost)
# =============================================================================

class MinimalDenominator(IntEnum):
    """Compact representation of the 8 Validated Denominators"""
    FLUCTUATION   = 0
    BUDGET        = 1
    CONTRAST      = 2
    OSCILLATION   = 3
    OFFSET        = 4
    TRACEABILITY  = 5
    ORIGIN_VAULT  = 6
    DRIFT         = 7

class MinimalPhase(IntEnum):
    """Roller Coaster phases in minimal form"""
    COAST           = 0
    ASCENT          = 1
    PEAK            = 2
    DESCENT         = 3
    CRYSTALLIZATION = 4

# =============================================================================
# Minimal Action / Offset Representation
# =============================================================================

@dataclass(slots=True)
class MinimalOffsetAction:
    """Extremely compact action representation.

    Uses only integers. No strings in the hot path.
    Human-readable names are resolved only when surfacing to user or logs.
    """
    action_id: int                  # Compact ID (0-255 for most common actions)
    intensity: Intensity8           # How strongly to apply (0-255)
    denominator: MinimalDenominator # Which denominator is driving this
    phase: MinimalPhase             # Current Roller Coaster phase context

    # Optional: only populated when we actually need to explain to a human
    _rationale_id: int | None = None

# =============================================================================
# Helper: Footprint Calculator (for NeuroBalance to enforce)
# =============================================================================

def compute_action_footprint(action: MinimalOffsetAction) -> int:
    """
    Returns an estimated 'compute cost' in abstract units.
    Lower is better. The NeuroBalance Coordinator can use this to prefer
    cheaper equivalent actions when multiple options exist.
    """
    base = 1
    if action.denominator in (MinimalDenominator.FLUCTUATION, MinimalDenominator.DRIFT):
        base += 2  # These often need slightly more history
    intensity_cost = (action.intensity >> 5) + 1  # 1-9 range
    return base + intensity_cost

# Example usage in a real Offset Engine:
# if compute_action_footprint(action_a) < compute_action_footprint(action_b):
#     prefer action_a