#!/usr/bin/env python3
from pathlib import Path
import runpy

root = Path(__file__).resolve().parents[1]
patcher = root / "tools" / "apply_v022_layout_audio_patch.py"
text = patcher.read_text(encoding="utf-8")

old_order = '''replace_all(".github/workflows/build.yml", "v0.21", "v0.22")
replace_all(".github/workflows/build.yml", "v0.21.0", "v0.22.0")'''
new_order = '''replace_all(".github/workflows/build.yml", "v0.21.0", "v0.22.0")
replace_all(".github/workflows/build.yml", "v0.21", "v0.22")'''
if old_order in text:
    text = text.replace(old_order, new_order, 1)

old_android = 'replace_all(".github/workflows/android.yml", "v0.21", "v0.22")'
new_android = '''if "v0.21" in read(".github/workflows/android.yml"):
    replace_all(".github/workflows/android.yml", "v0.21", "v0.22")'''
if old_android in text:
    text = text.replace(old_android, new_android, 1)

patcher.write_text(text, encoding="utf-8")
runpy.run_path(str(patcher), run_name="__main__")
