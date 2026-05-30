#!/usr/bin/env python3
"""
NeuroBalance Engine

Purpose:
    Detect non-clinical behavioral/cognitive operating states from user-provided
    signals and recommend low-risk support actions.

Core design:
    - Primitives are not "declared"; they are observed through signals.
    - Relationships between primitives create mechanisms.
    - Mechanisms earn confidence through recurrence, stability, and usefulness.
    - Output remains advisory unless explicitly wired to execution systems.

No external dependencies.
"""

from __future__ import annotations

import argparse
import json
import math
import statistics
import time
from collections import Counter, defaultdict, deque
from dataclasses import dataclass, field, asdict
from enum import Enum
from typing import Any, Deque, Dict, List, Optional, Tuple


# =============================================================================
# Enums
# =============================================================================

class EventType(str, Enum):
    ACTIVITY = "activity"
    APP_SWITCH = "app_switch"
    SELF_REPORT = "self_report"
    BIOMETRIC = "biometric"
    TEXT_SIGNAL = "text_signal"
    PATCH = "patch"
    BREAK = "break"
    TASK_PROGRESS = "task_progress"


class CandidateState(str, Enum):
    BALANCED = "balanced"
    HYPERFOCUS = "hyperfocus"
    COGNITIVE_OVERLOAD = "cognitive_overload"
    ANXIETY_SPIKE = "anxiety_spike"
    FATIGUE = "fatigue"
    RUMINATION_LOOP = "rumination_loop"
    CONTEXT_DRIFT = "context_drift"
    UNKNOWN = "unknown"


class LensMode(str, Enum):
    MIRROR = "mirror"
    COUNTER = "counter"
    ORTHOGONAL = "orthogonal"
    SYNTH = "synth"
    OBSERVER = "observer"


class Tint(str, Enum):
    GAP_IDENTIFIER = "gap_identifier"
    GAP_RESOLVER = "gap_resolver"


class OperatingPlane(str, Enum):
    ANALYSIS = "analysis"
    EXECUTE = "execute"


# =============================================================================
# Data Models
# =============================================================================

@dataclass
class SignalEvent:
    """
    Generic event object.

    Examples:
        EventType.ACTIVITY:
            value = {
                "keystrokes_per_min": 80,
                "mouse_events_per_min": 40,
                "active_minutes": 120
            }

        EventType.SELF_REPORT:
            value = {
                "stress": 8,
                "fatigue": 3,
                "focus": 9,
                "overload": 6,
                "note": "I cannot stop working on this"
            }

        EventType.BIOMETRIC:
            value = {
                "heart_rate": 104,
                "baseline_heart_rate": 72
            }
    """
    event_type: EventType
    timestamp: float
    value: Dict[str, Any]


@dataclass
class Primitive:
    name: str
    primitive_type: str  # atomic, kinetic, fractal
    evidence: List[str] = field(default_factory=list)
    confidence: float = 0.0


@dataclass
class Mechanism:
    name: str
    primitives: List[str]
    relationship: str
    input_signal: str
    transformation: str
    output_effect: str
    confidence: float
    earned: bool = False
    evidence: List[str] = field(default_factory=list)


@dataclass
class Gap:
    name: str
    description: str
    severity: float
    evidence: List[str]


@dataclass
class SupportAction:
    name: str
    action_type: str
    description: str
    risk_level: str
    execute_allowed: bool
    rationale: List[str]


@dataclass
class StateAssessment:
    primary_state: CandidateState
    state_scores: Dict[str, float]
    confidence: float
    gaps: List[Gap]
    mechanisms: List[Mechanism]
    recommended_actions: List[SupportAction]
    operating_plane: OperatingPlane
    explanation: str


# =============================================================================
# Rolling Window
# =============================================================================

class RollingEventWindow:
    def __init__(self, max_events: int = 5000, window_seconds: int = 4 * 60 * 60):
        self.max_events = max_events
        self.window_seconds = window_seconds
        self.events: Deque[SignalEvent] = deque(maxlen=max_events)

    def add(self, event: SignalEvent) -> None:
        self.events.append(event)
        self._trim_old_events()

    def _trim_old_events(self) -> None:
        now = time.time()
        cutoff = now - self.window_seconds
        while self.events and self.events[0].timestamp < cutoff:
            self.events.popleft()

    def recent(self, seconds: Optional[int] = None) -> List[SignalEvent]:
        self._trim_old_events()
        if seconds is None:
            return list(self.events)
        cutoff = time.time() - seconds
        return [e for e in self.events if e.timestamp >= cutoff]

    def by_type(self, event_type: EventType, seconds: Optional[int] = None) -> List[SignalEvent]:
        return [e for e in self.recent(seconds) if e.event_type == event_type]


# =============================================================================
# Corpus / Patch Stream Stats
# =============================================================================

class PatchStreamStats:
    """
    Tracks whether primitives/mechanisms/actions are actually earning their place.

    This is the "earned, not declared" layer.
    """

    def __init__(self):
        self.primitive_hits: Counter[str] = Counter()
        self.mechanism_hits: Counter[str] = Counter()
        self.action_outcomes: Dict[str, List[float]] = defaultdict(list)
        self.patch_notes: List[str] = []

    def record_primitive(self, primitive_name: str) -> None:
        self.primitive_hits[primitive_name] += 1

    def record_mechanism(self, mechanism_name: str) -> None:
        self.mechanism_hits[mechanism_name] += 1

    def record_action_outcome(self, action_name: str, usefulness_score: float) -> None:
        """
        usefulness_score: 0.0 to 1.0
        """
        usefulness_score = max(0.0, min(1.0, usefulness_score))
        self.action_outcomes[action_name].append(usefulness_score)

    def ingest_patch_note(self, note: str) -> None:
        self.patch_notes.append(note[:1000])

    def mechanism_is_earned(self, mechanism_name: str, min_hits: int = 3) -> bool:
        return self.mechanism_hits[mechanism_name] >= min_hits

    def action_is_earned(self, action_name: str, min_uses: int = 3, min_avg_score: float = 0.65) -> bool:
        scores = self.action_outcomes.get(action_name, [])
        if len(scores) < min_uses:
            return False
        return statistics.mean(scores) >= min_avg_score

    def snapshot(self) -> Dict[str, Any]:
        return {
            "primitive_hits": dict(self.primitive_hits),
            "mechanism_hits": dict(self.mechanism_hits),
            "action_outcomes": {
                k: {
                    "uses": len(v),
                    "avg_usefulness": round(statistics.mean(v), 3) if v else None,
                }
                for k, v in self.action_outcomes.items()
            },
            "patch_notes_count": len(self.patch_notes),
        }


# =============================================================================
# Utility Functions
# =============================================================================

def clamp(value: float, low: float = 0.0, high: float = 1.0) -> float:
    return max(low, min(high, value))


def safe_num(value: Any, default: float = 0.0) -> float:
    try:
        if value is None:
            return default
        return float(value)
    except (TypeError, ValueError):
        return default


def normalize(value: float, low: float, high: float) -> float:
    if high <= low:
        return 0.0
    return clamp((value - low) / (high - low))


def keyword_hits(text: str, keywords: List[str]) -> int:
    text_lower = text.lower()
    return sum(1 for k in keywords if k.lower() in text_lower)


# =============================================================================
# Detector Engine
# =============================================================================

class NeuroBalanceEngine:
    def __init__(
        self,
        operating_plane: OperatingPlane = OperatingPlane.ANALYSIS,
        lens: LensMode = LensMode.OBSERVER,
        anchor: LensMode = LensMode.SYNTH,
        tint: Tint = Tint.GAP_IDENTIFIER,
    ):
        self.window = RollingEventWindow()
        self.stats = PatchStreamStats()
        self.operating_plane = operating_plane
        self.lens = lens
        self.anchor = anchor
        self.tint = tint

    # -------------------------------------------------------------------------
    # Ingest
    # -------------------------------------------------------------------------

    def ingest(self, event: SignalEvent) -> None:
        self.window.add(event)

        if event.event_type == EventType.PATCH:
            note = str(event.value.get("note", ""))
            if note:
                self.stats.ingest_patch_note(note)

    # -------------------------------------------------------------------------
    # Main Assessment
    # -------------------------------------------------------------------------

    def assess(self) -> StateAssessment:
        primitives = self._extract_primitives()
        for p in primitives:
            self.stats.record_primitive(p.name)

        mechanisms = self._derive_mechanisms(primitives)
        for m in mechanisms:
            self.stats.record_mechanism(m.name)
            m.earned = self.stats.mechanism_is_earned(m.name)

        state_scores = self._score_states()
        primary_state, confidence = self._select_primary_state(state_scores)

        gaps = self._identify_gaps(primary_state, state_scores)
        actions = self._recommend_actions(primary_state, gaps, mechanisms)

        explanation = self._build_explanation(
            primary_state=primary_state,
            confidence=confidence,
            primitives=primitives,
            mechanisms=mechanisms,
            gaps=gaps,
        )

        return StateAssessment(
            primary_state=primary_state,
            state_scores={k.value: round(v, 3) for k, v in state_scores.items()},
            confidence=round(confidence, 3),
            gaps=gaps,
            mechanisms=mechanisms,
            recommended_actions=actions,
            operating_plane=self.operating_plane,
            explanation=explanation,
        )

    # -------------------------------------------------------------------------
    # Primitive Extraction
    # -------------------------------------------------------------------------

    def _extract_primitives(self) -> List[Primitive]:
        events = self.window.recent()
        primitives: List[Primitive] = []

        active_minutes = self._active_minutes()
        app_switches_30m = len(self.window.by_type(EventType.APP_SWITCH, 30 * 60))
        breaks_2h = len(self.window.by_type(EventType.BREAK, 2 * 60 * 60))
        progress_2h = self._task_progress_score(2 * 60 * 60)
        self_report = self._latest_self_report()
        avg_hr_delta = self._avg_heart_rate_delta()

        # Atomic primitives
        if active_minutes >= 60:
            primitives.append(Primitive(
                name="sustained_attention",
                primitive_type="atomic",
                confidence=normalize(active_minutes, 45, 150),
                evidence=[f"active_minutes={active_minutes:.1f}"],
            ))

        if app_switches_30m >= 12:
            primitives.append(Primitive(
                name="context_switching",
                primitive_type="atomic",
                confidence=normalize(app_switches_30m, 10, 40),
                evidence=[f"app_switches_30m={app_switches_30m}"],
            ))

        if self_report:
            stress = safe_num(self_report.get("stress"))
            fatigue = safe_num(self_report.get("fatigue"))
            overload = safe_num(self_report.get("overload"))
            focus = safe_num(self_report.get("focus"))

            if stress >= 7:
                primitives.append(Primitive(
                    name="stress_signal",
                    primitive_type="atomic",
                    confidence=normalize(stress, 5, 10),
                    evidence=[f"self_report.stress={stress}"],
                ))

            if fatigue >= 7:
                primitives.append(Primitive(
                    name="fatigue_signal",
                    primitive_type="atomic",
                    confidence=normalize(fatigue, 5, 10),
                    evidence=[f"self_report.fatigue={fatigue}"],
                ))

            if overload >= 7:
                primitives.append(Primitive(
                    name="overload_signal",
                    primitive_type="atomic",
                    confidence=normalize(overload, 5, 10),
                    evidence=[f"self_report.overload={overload}"],
                ))

            if focus >= 8:
                primitives.append(Primitive(
                    name="high_focus_signal",
                    primitive_type="atomic",
                    confidence=normalize(focus, 6, 10),
                    evidence=[f"self_report.focus={focus}"],
                ))

            note = str(self_report.get("note", ""))
            if keyword_hits(note, ["stuck", "loop", "again", "can't stop", "same thought", "spiral"]) >= 1:
                primitives.append(Primitive(
                    name="loop_language",
                    primitive_type="fractal",
                    confidence=0.75,
                    evidence=[f"self_report.note={note[:120]}"],
                ))

        # Kinetic primitives
        if active_minutes >= 90 and breaks_2h == 0:
            primitives.append(Primitive(
                name="break_suppression",
                primitive_type="kinetic",
                confidence=normalize(active_minutes, 80, 180),
                evidence=[f"active_minutes={active_minutes:.1f}", f"breaks_2h={breaks_2h}"],
            ))

        if avg_hr_delta >= 20:
            primitives.append(Primitive(
                name="physiological_activation",
                primitive_type="kinetic",
                confidence=normalize(avg_hr_delta, 15, 45),
                evidence=[f"avg_heart_rate_delta={avg_hr_delta:.1f}"],
            ))

        if progress_2h < 0.25 and active_minutes >= 60:
            primitives.append(Primitive(
                name="effort_without_progress",
                primitive_type="kinetic",
                confidence=clamp(1.0 - progress_2h),
                evidence=[f"progress_2h={progress_2h:.2f}", f"active_minutes={active_minutes:.1f}"],
            ))

        # Fractal primitives
        text_events = self.window.by_type(EventType.TEXT_SIGNAL, 2 * 60 * 60)
        combined_text = " ".join(str(e.value.get("text", "")) for e in text_events)
        if keyword_hits(combined_text, ["gap", "missing", "why", "again", "same", "loop"]) >= 3:
            primitives.append(Primitive(
                name="recursive_gap_attention",
                primitive_type="fractal",
                confidence=0.7,
                evidence=["repeated gap/loop language in text stream"],
            ))

        return primitives

    # -------------------------------------------------------------------------
    # Mechanism Derivation
    # -------------------------------------------------------------------------

    def _derive_mechanisms(self, primitives: List[Primitive]) -> List[Mechanism]:
        names = {p.name for p in primitives}
        mechanisms: List[Mechanism] = []

        def add(
            name: str,
            prims: List[str],
            relationship: str,
            input_signal: str,
            transformation: str,
            output_effect: str,
            confidence: float,
            evidence: List[str],
        ) -> None:
            mechanisms.append(Mechanism(
                name=name,
                primitives=prims,
                relationship=relationship,
                input_signal=input_signal,
                transformation=transformation,
                output_effect=output_effect,
                confidence=round(clamp(confidence), 3),
                evidence=evidence,
            ))

        if {"sustained_attention", "break_suppression", "high_focus_signal"} <= names:
            add(
                name="hyperfocus_lock",
                prims=["sustained_attention", "break_suppression", "high_focus_signal"],
                relationship="sustained attention reinforced by break suppression",
                input_signal="long active session with low interruption",
                transformation="attention narrows and resists disengagement",
                output_effect="high productivity potential with depletion risk",
                confidence=0.86,
                evidence=["sustained attention + no breaks + high focus"],
            )

        if {"context_switching", "overload_signal"} <= names:
            add(
                name="switching_overload",
                prims=["context_switching", "overload_signal"],
                relationship="rapid switching amplifies overload",
                input_signal="high app/task switching and overload report",
                transformation="working memory fragmentation",
                output_effect="reduced clarity and increased agitation risk",
                confidence=0.82,
                evidence=["context switching + overload signal"],
            )

        if {"stress_signal", "physiological_activation"} <= names:
            add(
                name="activation_spike",
                prims=["stress_signal", "physiological_activation"],
                relationship="subjective stress aligned with body activation",
                input_signal="stress report plus elevated biometric delta",
                transformation="arousal crosses support threshold",
                output_effect="grounding or de-escalation recommended",
                confidence=0.84,
                evidence=["stress signal + physiological activation"],
            )

        if {"effort_without_progress", "loop_language"} <= names:
            add(
                name="rumination_capture",
                prims=["effort_without_progress", "loop_language"],
                relationship="effort loops without closure",
                input_signal="work continues but progress remains low",
                transformation="attention recycles around unresolved gap",
                output_effect="externalize loop and define next smallest action",
                confidence=0.8,
                evidence=["effort without progress + loop language"],
            )

        if {"recursive_gap_attention", "effort_without_progress"} <= names:
            add(
                name="gap_recursion",
                prims=["recursive_gap_attention", "effort_without_progress"],
                relationship="fractal gap attention consumes kinetic effort",
                input_signal="repeated gap framing with low progress",
                transformation="gap identification outruns gap resolution",
                output_effect="resolver tint should be activated",
                confidence=0.78,
                evidence=["recursive gap language + low progress"],
            )

        if {"fatigue_signal", "break_suppression"} <= names:
            add(
                name="depletion_risk",
                prims=["fatigue_signal", "break_suppression"],
                relationship="fatigue persists while recovery is suppressed",
                input_signal="fatigue report and lack of breaks",
                transformation="recovery deficit accumulates",
                output_effect="break or task downshift recommended",
                confidence=0.86,
                evidence=["fatigue signal + no breaks"],
            )

        return mechanisms

    # ---------------------------------------------------------------------------------------------------------------
    # State Scoring
    # -------------------------------------------------------------------------

    def _score_states(self) -> Dict[CandidateState, float]:
        active_minutes = self._active_minutes()
        breaks_2h = len(self.window.by_type(EventType.BREAK, 2 * 60 * 60))
        app_switches_30m = len(self.window.by_type(EventType.APP_SWITCH, 30 * 60))
        progress_2h = self._task_progress_score(2 * 60 * 60)
        self_report = self._latest_self_report()
        avg_hr_delta = self._avg_heart_rate_delta()

        stress = safe_num(self_report.get("stress")) if self_report else 0.0
        fatigue = safe_num(self_report.get("fatigue")) if self_report else 0.0
        overload = safe_num(self_report.get("overload")) if self_report else 0.0
        focus = safe_num(self_report.get("focus")) if self_report else 0.0
        note = str(self_report.get("note", "")) if self_report else ""

        loop_note_score = normalize(keyword_hits(note, [
            "stuck", "loop", "again", "spiral", "same thought", "can't stop"
        ]), 0, 3)

        hyperfocus = (
            0.35 * normalize(active_minutes, 60, 180)
            + 0.25 * normalize(focus, 6, 10)
            + 0.25 * (1.0 if breaks_2h == 0 and active_minutes >= 75 else 0.0)
            + 0.15 * (1.0 - normalize(app_switches_30m, 5, 30))
        )

        overload_score = (
            0.35 * normalize(overload, 4, 10)
            + 0.30 * normalize(app_switches_30m, 8, 40)
            + 0.20 * normalize(stress, 5, 10)
            + 0.15 * (1.0 - progress_2h)
        )

        anxiety_spike = (
            0.45 * normalize(stress, 5, 10)
            + 0.35 * normalize(avg_hr_delta, 10, 45)
            + 0.20 * loop_note_score
        )

        fatigue_score = (
            0.50 * normalize(fatigue, 5, 10)
            + 0.25 * normalize(active_minutes, 90, 240)
            + 0.25 * (1.0 if breaks_2h == 0 and active_minutes >= 90 else 0.0)
        )

        rumination = (
            0.40 * loop_note_score
            + 0.35 * (1.0 - progress_2h if active_minutes >= 45 else 0.0)
            + 0.25 * normalize(active_minutes, 60, 180)
        )

        context_drift = (
            0.55 * normalize(app_switches_30m, 10, 45)
            + 0.30 * (1.0 - progress_2h)
            + 0.15 * normalize(overload, 5, 10)
        )

        balanced = 1.0 - max(
            hyperfocus,
            overload_score,
            anxiety_spike,
            fatigue_score,
            rumination,
            context_drift,
        )

        return {
            CandidateState.HYPERFOCUS: clamp(hyperfocus),
            CandidateState.COGNITIVE_OVERLOAD: clamp(overload_score),
            CandidateState.ANXIETY_SPIKE: clamp(anxiety_spike),
            CandidateState.FATIGUE: clamp(fatigue_score),
            CandidateState.RUMINATION_LOOP: clamp(rumination),
            CandidateState.CONTEXT_DRIFT: clamp(context_drift),
            CandidateState.BALANCED: clamp(balanced),
        }

    def _select_primary_state(self, scores: Dict[CandidateState, float]) -> Tuple[CandidateState, float]:
        ranked = sorted(scores.items(), key=lambda kv: kv[1], reverse=True)
        primary, score = ranked[0]

        if score < 0.35:
            return CandidateState.UNKNOWN, round(score, 3)

        return primary, round(score, 3)

    # -------------------------------------------------------------------------
    # Gap Identification
    # -------------------------------------------------------------------------

    def _identify_gaps(
        self,
        primary_state: CandidateState,
        scores: Dict[CandidateState, float],
    ) -> List[Gap]:
        gaps: List[Gap] = []

        def add(name: str, description: str, severity: float, evidence: List[str]) -> None:
            gaps.append(Gap(
                name=name,
                description=description,
                severity=round(clamp(severity), 3),
                evidence=evidence,
            ))

        if primary_state == CandidateState.HYPERFOCUS:
            add(
                name="recovery_gap",
                description="Sustained focus is not being balanced by recovery checkpoints.",
                severity=scores[CandidateState.HYPERFOCUS],
                evidence=["hyperfocus score is primary"],
            )

        if primary_state == CandidateState.COGNITIVE_OVERLOAD:
            add(
                name="compression_gap",
                description="Too many active inputs need to be compressed into fewer working targets.",
                severity=scores[CandidateState.COGNITIVE_OVERLOAD],
                evidence=["overload/context switching pattern detected"],
            )

        if primary_state == CandidateState.ANXIETY_SPIKE:
            add(
                name="regulation_gap",
                description="Stress activation appears elevated and needs a low-friction settling step.",
                severity=scores[CandidateState.ANXIETY_SPIKE],
                evidence=["stress and/or physiological activation pattern detected"],
            )

        if primary_state == CandidateState.FATIGUE:
            add(
                name="energy_gap",
                description="Current task demand appears misaligned with available energy.",
                severity=scores[CandidateState.FATIGUE],
                evidence=["fatigue and/or long-session pattern detected"],
            )

        if primary_state == CandidateState.RUMINATION_LOOP:
            add(
                name="closure_gap",
                description="Attention appears to be circling an unresolved issue without a closure action.",
                severity=scores[CandidateState.RUMINATION_LOOP],
                evidence=["loop language and low progress detected"],
            )

        if primary_state == CandidateState.CONTEXT_DRIFT:
            add(
                name="anchor_gap",
                description="The active objective needs a clearer anchor to reduce task switching.",
                severity=scores[CandidateState.CONTEXT_DRIFT],
                evidence=["high switching and low progress detected"],
            )

        return gaps

    # -------------------------------------------------------------------------
    # Action Recommendations
    # -------------------------------------------------------------------------

    def _recommend_actions(
        self,
        primary_state: CandidateState,
        gaps: List[Gap],
        mechanisms: List[Mechanism],
    ) -> List[SupportAction]:

        actions: List[SupportAction] = []

        def add(
            name: str,
            action_type: str,
            description: str,
            risk_level: str,
            execute_allowed: bool,
            rationale: List[str],
        ) -> None:
            # Hard safety rule:
            # If operating plane is analysis, do not execute.
            if self.operating_plane == OperatingPlane.ANALYSIS:
                execute_allowed = False

            actions.append(SupportAction(
                name=name,
                action_type=action_type,
                description=description,
                risk_level=risk_level,
                execute_allowed=execute_allowed,
                rationale=rationale,
            ))

        mech_names = {m.name for m in mechanisms}

        if primary_state == CandidateState.HYPERFOCUS:
            add(
                name="micro_break_checkpoint",
                action_type="nudge",
                description=(
                    "Pause for 3 minutes. Drink water, stand up, and write one sentence: "
                    "'The next useful action is ____.'"
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["hyperfocus can be productive but needs recovery boundaries"],
            )

            add(
                name="define_exit_condition",
                action_type="structure",
                description=(
                    "Set a concrete stop condition: time limit, completed subtask, or clear handoff point."
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["prevents open-ended focus lock"],
            )

        elif primary_state == CandidateState.COGNITIVE_OVERLOAD:
            add(
                name="compress_inputs_to_three",
                action_type="reduction",
                description=(
                    "List every active concern, then choose only three: current objective, blocker, next action."
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["reduces working-memory load"],
            )

            add(
                name="single_tab_protocol",
                action_type="environment",
                description=(
                    "Close or park everything except the one surface needed for the next action."
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["limits context switching"],
            )

        elif primary_state == CandidateState.ANXIETY_SPIKE:
            add(
                name="grounding_sequence",
                action_type="regulation",
                description=(
                    "Run a short grounding sequence: name five visible objects, four physical sensations, "
                    "three sounds, two next actions, and one immediate safe choice."
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["supports non-clinical regulation without diagnosis"],
            )

            add(
                name="delay_execution",
                action_type="safety_brake",
                description=(
                    "Keep decisions in analysis mode until the activation score drops or a second review confirms action."
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["high activation can degrade execution quality"],
            )

        elif primary_state == CandidateState.FATIGUE:
            add(
                name="task_downshift",
                action_type="energy_management",
                description=(
                    "Switch from creation to maintenance: rename files, organize notes, review checklist, "
                    "or capture tomorrow's first step."
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["matches task demand to current energy"],
            )

        elif primary_state == CandidateState.RUMINATION_LOOP:
            add(
                name="externalize_loop",
                action_type="closure",
                description=(
                    "Write the loop in this format: 'I keep returning to __ because __. "
                    "The smallest closure action is __.'"
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["turns recursive thought into a bounded object"],
            )

            add(
                name="resolver_tint_switch",
                action_type="lens_shift",
                description=(
                    "Switch from gap identifier to gap resolver for one cycle. No new problems. Only one bridge."
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["gap identification may be outrunning gap resolution"],
            )

        elif primary_state == CandidateState.CONTEXT_DRIFT:
            add(
                name="objective_anchor",
                action_type="anchor",
                description=(
                    "Write the current objective in one sentence. Then write the next physical action in one verb."
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["reduces drift by anchoring the active task"],
            )

        else:
            add(
                name="observer_check",
                action_type="assessment",
                description=(
                    "No strong state detected. Continue observation and collect more signal before intervening."
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["insufficient evidence for stronger recommendation"],
            )

        # Mechanism-specific refinement
        if "gap_recursion" in mech_names:
            add(
                name="gap_to_mechanism_conversion",
                action_type="framework",
                description=(
                    "Convert the gap into mechanism form: input, relationship, transformation, output, boundary."
                ),
                risk_level="low",
                execute_allowed=False,
                rationale=["gap recursion detected"],
            )

        return actions

    # -------------------------------------------------------------------------
    # Explanation
    # -------------------------------------------------------------------------

    def _build_explanation(
        self,
        primary_state: CandidateState,
        confidence: float,
        primitives: List[Primitive],
        mechanisms: List[Mechanism],
        gaps: List[Gap],
    ) -> str:
        primitive_names = [p.name for p in primitives]
        mechanism_names = [m.name for m in mechanisms]
        gap_names = [g.name for g in gaps]

        return (
            f"Lens={self.lens.value} anchored_by={self.anchor.value}, tint={self.tint.value}. "
            f"Primary state is {primary_state.value} with confidence {confidence}. "
            f"Observed primitives: {primitive_names}. "
            f"Derived mechanisms: {mechanism_names}. "
            f"Identified gaps: {gap_names}. "
            f"Operating plane is {self.operating_plane.value}; advisory output only unless execute mode is explicitly enabled."
        )

    # -------------------------------------------------------------------------
    # Signal Helpers
    # -------------------------------------------------------------------------

    def _latest_self_report(self) -> Optional[Dict[str, Any]]:
        reports = self.window.by_type(EventType.SELF_REPORT)
        if not reports:
            return None
        return reports[-1].value

    def _active_minutes(self) -> float:
        activity_events = self.window.by_type(EventType.ACTIVITY, 4 * 60 * 60)
        if not activity_events:
            return 0.0

        # Prefer explicit active_minutes from latest event when present.
        latest = activity_events[-1].value
        if "active_minutes" in latest:
            return safe_num(latest.get("active_minutes"))

        # Otherwise estimate active span.
        timestamps = [e.timestamp for e in activity_events]
        return max(0.0, (max(timestamps) - min(timestamps)) / 60.0)

    def _task_progress_score(self, seconds: int) -> float:
        progress_events = self.window.by_type(EventType.TASK_PROGRESS, seconds)
        if not progress_events:
            return 0.5  # Unknown, neutral.

        vals = [clamp(safe_num(e.value.get("progress"), 0.0)) for e in progress_events]
        if not vals:
            return 0.5
        return clamp(statistics.mean(vals))

    def _avg_heart_rate_delta(self) -> float:
        biometric_events = self.window.by_type(EventType.BIOMETRIC, 60 * 60)
        deltas = []

        for e in biometric_events:
            hr = safe_num(e.value.get("heart_rate"))
            base = safe_num(e.value.get("baseline_heart_rate"))
            if hr > 0 and base > 0:
                deltas.append(hr - base)

        if not deltas:
            return 0.0

        return statistics.mean(deltas)


# =============================================================================
# Serialization
# =============================================================================

def assessment_to_json(assessment: StateAssessment, stats: PatchStreamStats) -> str:
    payload = asdict(assessment)
    payload["corpus_stats"] = stats.snapshot()
    return json.dumps(payload, indent=2)


# =============================================================================
# Demo
# =============================================================================

def demo() -> None:
    engine = NeuroBalanceEngine(
        operating_plane=OperatingPlane.ANALYSIS,
        lens=LensMode.OBSERVER,
        anchor=LensMode.SYNTH,
        tint=Tint.GAP_IDENTIFIER,
    )

    now = time.time()

    demo_events = [
        SignalEvent(
            event_type=EventType.ACTIVITY,
            timestamp=now - 110 * 60,
            value={
                "keystrokes_per_min": 75,
                "mouse_events_per_min": 35,
                "active_minutes": 110,
            },
        ),
        SignalEvent(
            event_type=EventType.TASK_PROGRESS,
            timestamp=now - 60 * 60,
            value={"progress": 0.18},
        ),
        SignalEvent(
            event_type=EventType.SELF_REPORT,
            timestamp=now - 5 * 60,
            value={
                "stress": 6,
                "fatigue": 4,
                "focus": 9,
                "overload": 5,
                "note": "I cannot stop working on this and I keep circling the same gap.",
            },
        ),
        SignalEvent(
            event_type=EventType.PATCH,
            timestamp=now - 2 * 60,
            value={
                "note": "Patch: hyperfocus lock appeared again when active minutes exceeded 90 and no break was recorded."
            },
        ),
    ]

    # Add app-switch events
    for i in range(6):
        demo_events.append(
            SignalEvent(
                event_type=EventType.APP_SWITCH,
                timestamp=now - (20 * 60) + i * 120,
                value={"from": "editor", "to": "browser"},
            )
        )

    for event in demo_events:
        engine.ingest(event)

    assessment = engine.assess()
    print(assessment_to_json(assessment, engine.stats))


# =============================================================================
# CLI
# =============================================================================

def main() -> None:
    parser = argparse.ArgumentParser(description="NeuroBalance Engine")
    parser.add_argument("--demo", action="store_true", help="Run built-in demo")
    parser.add_argument(
        "--event-json",
        type=str,
        default=None,
        help=(
            "Optional JSON event to ingest once. Example: "
            "'{\"event_type\":\"self_report\",\"value\":{\"stress\":8,\"focus\":9}}'"
        ),
    )
    args = parser.parse_args()

    if args.demo:
        demo()
        return

    engine = NeuroBalanceEngine()

    if args.event_json:
        raw = json.loads(args.event_json)
        event = SignalEvent(
            event_type=EventType(raw["event_type"]),
            timestamp=safe_num(raw.get("timestamp"), time.time()),
            value=raw.get("value", {}),
        )
        engine.ingest(event)
        assessment = engine.assess()
        print(assessment_to_json(assessment, engine.stats))
        return

    parser.print_help()


if __name__ == "__main__":
    main()
