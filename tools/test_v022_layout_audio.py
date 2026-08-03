#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

def text(path):
    return (root / path).read_text(encoding="utf-8")

def require(condition, message):
    if not condition:
        raise SystemExit(message)

audio = text("src/audio_v021.c")
require("request_rom_control(audio, events, false);" in audio,
        "sound effects must preserve queued music")
require("request_rom_control(audio, events, true);" not in audio,
        "sound effects still flush the music ring")
require("TETRIS_AUDIO_RING_TARGET_DEFAULT 4096u" in text("src/audio.h"),
        "unexpected default ring target")
manifest = text("android/app/src/main/AndroidManifest.xml")
require(manifest.count('android:screenOrientation="portrait"') >= 2,
        "Android activities are not portrait")
require('android:screenOrientation="landscape"' not in manifest,
        "landscape orientation remains in Android manifest")
require("#define DISPLAY_LOGICAL_H 1280" in text("src/app.h"),
        "Android handheld canvas is missing")
require("#define TOUCH_LOGICAL_H 1280" in text("src/touch_controls.c"),
        "Android touch canvas is missing")
main = text("src/main_v05_02.inc")
require("Tetris NES PC Port v0.22" in main,
        "visible desktop version is stale")
require("SDL_RenderSetLogicalSize(renderer, LOGICAL_W, DISPLAY_LOGICAL_H);" in main,
        "platform display height is not used")
settings = text("src/settings.c")
require("tetris_settings_fit_window_4_3" in settings,
        "PC 4:3 normalization is missing")
print("v0.22 audio/layout source tests: OK")
