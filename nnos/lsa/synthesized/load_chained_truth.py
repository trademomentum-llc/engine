#!/usr/bin/env python3
"""
Chained Source of Truth Loader for Kimi Agent (and other parallel agents)

Usage:
    python load_chained_truth.py

This script outputs (or can be extended to write) the current canonical
Chained Source of Truth document so it can be injected into another agent's
context.

The Kimi agent should load the contents of:
    engine/nnos/lsa/synthesized/Chained_Source_of_Truth_Kimi_Binding.md

at the beginning of relevant sessions.
"""

import os
from pathlib import Path

BINDING_FILE = Path(__file__).parent / "Chained_Source_of_Truth_Kimi_Binding.md"

def load_chained_truth() -> str:
    if not BINDING_FILE.exists():
        return "ERROR: Chained Source of Truth binding file not found."
    
    content = BINDING_FILE.read_text(encoding="utf-8")
    return content

if __name__ == "__main__":
    print("=" * 80)
    print("NEURODIOS CHAINED SOURCE OF TRUTH — LOADED FOR KIMI AGENT")
    print("=" * 80)
    print()
    print(load_chained_truth())
    print()
    print("=" * 80)
    print("End of Chained Source of Truth payload.")
    print("Instruct the Kimi agent to treat the above as its persistent memory layer.")