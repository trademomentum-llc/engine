#!/usr/bin/env python3
import sys
from pathlib import Path

# The real runner and shared library live in System/apps/scripts/
APPS_SCRIPTS = Path(__file__).resolve().parent.parent.parent.parent.parent.parent / "apps" / "scripts"
sys.path.insert(0, str(APPS_SCRIPTS))

from jmk_regression import main  # noqa: E402

if __name__ == "__main__":
    kernel_dir = Path(__file__).resolve().parent.parent
    raise SystemExit(main(kernel_dir=kernel_dir))
