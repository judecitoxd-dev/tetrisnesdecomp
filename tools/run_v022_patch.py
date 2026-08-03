#!/usr/bin/env python3
from pathlib import Path
import runpy

# Rerun with diagnostics enabled by the temporary workflow.
root = Path(__file__).resolve().parents[1]
patcher = root / "tools" / "apply_v022_layout_audio_patch.py"
text = patcher.read_text(encoding="utf-8")
old = '''replace_all(".github/workflows/build.yml", "v0.21", "v0.22")
replace_all(".github/workflows/build.yml", "v0.21.0", "v0.22.0")'''
new = '''replace_all(".github/workflows/build.yml", "v0.21.0", "v0.22.0")
replace_all(".github/workflows/build.yml", "v0.21", "v0.22")'''
if old not in text:
    raise SystemExit("Could not find the workflow version replacement block")
patcher.write_text(text.replace(old, new, 1), encoding="utf-8")
runpy.run_path(str(patcher), run_name="__main__")
