#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_once(path: str, old: str, new: str) -> None:
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one match, found {count}: {old[:80]!r}")
    write(path, text.replace(old, new, 1))


def replace_all(path: str, old: str, new: str, minimum: int = 1) -> None:
    text = read(path)
    count = text.count(old)
    if count < minimum:
        raise RuntimeError(f"{path}: expected at least {minimum} matches for {old!r}")
    write(path, text.replace(old, new))


# Audio: SFX must not discard already queued music. A flush jumped the driver
# state ahead by the complete ring depth on every move/rotation/lock.
replace_once(
    "src/audio_v021.c",
    "        request_rom_control(audio, events, true);",
    "        request_rom_control(audio, events, false);",
)
replace_once(
    "src/audio.h",
    "#define TETRIS_AUDIO_RING_TARGET_DEFAULT 8192u",
    "#define TETRIS_AUDIO_RING_TARGET_DEFAULT 4096u",
)

# Separate the 4:3 NES canvas from Android's portrait handheld canvas.
replace_once(
    "src/app.h",
    "#define LOGICAL_W 640\n#define LOGICAL_H 480",
    "#define LOGICAL_W 640\n#define LOGICAL_H 480\n#ifdef __ANDROID__\n#define DISPLAY_LOGICAL_H 1280\n#else\n#define DISPLAY_LOGICAL_H LOGICAL_H\n#endif",
)

replace_once(
    "src/main_v05_02.inc",
    '    SDL_Window *window = SDL_CreateWindow("Tetris NES PC Port v0.20",',
    '    SDL_Window *window = SDL_CreateWindow("Tetris NES PC Port v0.22",',
)
replace_once(
    "src/main_v05_02.inc",
    "    SDL_RenderSetLogicalSize(renderer, LOGICAL_W, LOGICAL_H);\n"
    "    SDL_RenderSetIntegerScale(renderer,\n"
    "        settings.integer_scale ? SDL_TRUE : SDL_FALSE);",
    "    SDL_RenderSetLogicalSize(renderer, LOGICAL_W, DISPLAY_LOGICAL_H);\n"
    "#ifdef __ANDROID__\n"
    "    SDL_RenderSetIntegerScale(renderer, SDL_FALSE);\n"
    "#else\n"
    "    SDL_RenderSetIntegerScale(renderer,\n"
    "        settings.integer_scale ? SDL_TRUE : SDL_FALSE);\n"
    "#endif",
)
replace_once(
    "src/main_v05_02.inc",
    "            if (event.type == SDL_WINDOWEVENT &&\n"
    "                event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED &&\n"
    "                !settings.fullscreen) {\n"
    "                settings.window_width = event.window.data1;\n"
    "                settings.window_height = event.window.data2;\n"
    "            }",
    "            if (event.type == SDL_WINDOWEVENT &&\n"
    "                event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED &&\n"
    "                !settings.fullscreen) {\n"
    "                int adjusted_width = event.window.data1;\n"
    "                int adjusted_height = event.window.data2;\n"
    "#ifndef __ANDROID__\n"
    "                tetris_settings_fit_window_4_3(&adjusted_width,\n"
    "                                                &adjusted_height);\n"
    "                if (adjusted_width != event.window.data1 ||\n"
    "                    adjusted_height != event.window.data2)\n"
    "                    SDL_SetWindowSize(window, adjusted_width,\n"
    "                                      adjusted_height);\n"
    "#endif\n"
    "                settings.window_width = adjusted_width;\n"
    "                settings.window_height = adjusted_height;\n"
    "            }",
)

# Persistent PC size is normalized to an exact 4:3 integer multiple.
replace_once(
    "src/settings.h",
    "void tetris_settings_sanitize(TetrisSettings *settings);",
    "void tetris_settings_sanitize(TetrisSettings *settings);\n"
    "void tetris_settings_fit_window_4_3(int *width, int *height);",
)
replace_once(
    "src/settings.c",
    "void tetris_settings_init(TetrisSettings *settings) {",
    "void tetris_settings_fit_window_4_3(int *width, int *height) {\n"
    "    int unit_w;\n"
    "    int unit_h;\n"
    "    int unit;\n"
    "    if (!width || !height) return;\n"
    "    if (*width < 640) *width = 640;\n"
    "    if (*height < 480) *height = 480;\n"
    "    if (*width > 7680) *width = 7680;\n"
    "    if (*height > 4320) *height = 4320;\n"
    "    unit_w = *width / 4;\n"
    "    unit_h = *height / 3;\n"
    "    unit = unit_w < unit_h ? unit_w : unit_h;\n"
    "    if (unit < 160) unit = 160;\n"
    "    if (unit > 1440) unit = 1440;\n"
    "    *width = unit * 4;\n"
    "    *height = unit * 3;\n"
    "}\n\n"
    "void tetris_settings_init(TetrisSettings *settings) {",
)
replace_once(
    "src/settings.c",
    "    if (settings->window_height < 480) settings->window_height = 480;\n"
    "    if (settings->window_height > 4320) settings->window_height = 4320;",
    "    if (settings->window_height < 480) settings->window_height = 480;\n"
    "    if (settings->window_height > 4320) settings->window_height = 4320;\n"
    "#ifndef __ANDROID__\n"
    "    tetris_settings_fit_window_4_3(&settings->window_width,\n"
    "                                    &settings->window_height);\n"
    "#endif",
)

# Android-only portrait handheld layout. PC touch coordinates stay unchanged.
replace_once(
    "src/touch_controls.c",
    "#define TOUCH_LOGICAL_W 640\n#define TOUCH_LOGICAL_H 480",
    "#define TOUCH_LOGICAL_W 640\n"
    "#ifdef __ANDROID__\n"
    "#define TOUCH_LOGICAL_H 1280\n"
    "#else\n"
    "#define TOUCH_LOGICAL_H 480\n"
    "#endif",
)
replace_once(
    "src/touch_controls.c",
    "    static const int defaults[TETRIS_TOUCH_ACTION_COUNT][2] = {\n"
    "        {105, 326}, {105, 430}, {53, 378}, {157, 378},\n"
    "        {566, 330}, {494, 384}, {566, 428},\n"
    "        {370, 447}, {278, 447}, {48, 28}, {590, 28}\n"
    "    };",
    "#ifdef __ANDROID__\n"
    "    static const int defaults[TETRIS_TOUCH_ACTION_COUNT][2] = {\n"
    "        {145, 690}, {145, 890}, {55, 790}, {235, 790},\n"
    "        {535, 710}, {435, 820}, {535, 950},\n"
    "        {380, 1150}, {260, 1150}, {70, 545}, {570, 545}\n"
    "    };\n"
    "#else\n"
    "    static const int defaults[TETRIS_TOUCH_ACTION_COUNT][2] = {\n"
    "        {105, 326}, {105, 430}, {53, 378}, {157, 378},\n"
    "        {566, 330}, {494, 384}, {566, 428},\n"
    "        {370, 447}, {278, 447}, {48, 28}, {590, 28}\n"
    "    };\n"
    "#endif",
)
replace_once(
    "src/touch_controls.c",
    "static int base_radius(TetrisTouchAction action) {\n"
    "    switch (action) {\n"
    "        case TETRIS_TOUCH_START:\n"
    "        case TETRIS_TOUCH_SELECT:\n"
    "            return 30;\n"
    "        case TETRIS_TOUCH_ROM:\n"
    "        case TETRIS_TOUCH_EDIT:\n"
    "            return 26;\n"
    "        case TETRIS_TOUCH_DROP:\n"
    "            return 31;\n"
    "        default:\n"
    "            return 34;\n"
    "    }\n"
    "}",
    "static int base_radius(TetrisTouchAction action) {\n"
    "#ifdef __ANDROID__\n"
    "    switch (action) {\n"
    "        case TETRIS_TOUCH_START:\n"
    "        case TETRIS_TOUCH_SELECT:\n"
    "            return 34;\n"
    "        case TETRIS_TOUCH_ROM:\n"
    "        case TETRIS_TOUCH_EDIT:\n"
    "            return 28;\n"
    "        case TETRIS_TOUCH_DROP:\n"
    "            return 48;\n"
    "        case TETRIS_TOUCH_A:\n"
    "        case TETRIS_TOUCH_B:\n"
    "            return 54;\n"
    "        default:\n"
    "            return 50;\n"
    "    }\n"
    "#else\n"
    "    switch (action) {\n"
    "        case TETRIS_TOUCH_START:\n"
    "        case TETRIS_TOUCH_SELECT:\n"
    "            return 30;\n"
    "        case TETRIS_TOUCH_ROM:\n"
    "        case TETRIS_TOUCH_EDIT:\n"
    "            return 26;\n"
    "        case TETRIS_TOUCH_DROP:\n"
    "            return 31;\n"
    "        default:\n"
    "            return 34;\n"
    "    }\n"
    "#endif\n"
    "}",
)
replace_once(
    "src/touch_controls.c",
    "    return clamp_int(base_radius(action) * touch->scale / 100, 18, 62);",
    "#ifdef __ANDROID__\n"
    "    return clamp_int(base_radius(action) * touch->scale / 100, 22, 76);\n"
    "#else\n"
    "    return clamp_int(base_radius(action) * touch->scale / 100, 18, 62);\n"
    "#endif",
)
replace_once(
    "src/touch_controls.c",
    "    touch->opacity = 58;\n    touch->scale = 100;",
    "#ifdef __ANDROID__\n"
    "    touch->opacity = 86;\n"
    "    touch->scale = 110;\n"
    "#else\n"
    "    touch->opacity = 58;\n"
    "    touch->scale = 100;\n"
    "#endif",
)
replace_once(
    "src/touch_controls.c",
    "    touch->opacity = clamp_int(touch->opacity, 20, 100);\n"
    "    touch->scale = clamp_int(touch->scale, 60, 160);",
    "    touch->opacity = clamp_int(touch->opacity, 20, 100);\n"
    "    touch->scale = clamp_int(touch->scale, 60, 160);\n"
    "#ifdef __ANDROID__\n"
    "    if (touch->opacity < 68) touch->opacity = 68;\n"
    "    if (touch->scale < 100) touch->scale = 100;\n"
    "#endif",
)
replace_once(
    "src/touch_controls.c",
    "    SDL_SetRenderDrawColor(renderer, 245, 245, 245, fill_alpha);",
    "#ifdef __ANDROID__\n"
    "    if (action == TETRIS_TOUCH_A || action == TETRIS_TOUCH_B ||\n"
    "        action == TETRIS_TOUCH_DROP)\n"
    "        SDL_SetRenderDrawColor(renderer, 142, 35, 67, fill_alpha);\n"
    "    else\n"
    "        SDL_SetRenderDrawColor(renderer, 43, 46, 52, fill_alpha);\n"
    "#else\n"
    "    SDL_SetRenderDrawColor(renderer, 245, 245, 245, fill_alpha);\n"
    "#endif",
)
replace_once(
    "src/touch_controls.c",
    "    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);\n"
    "    alpha = (Uint8)clamp_int(touch->opacity * 255 / 100, 30, 255);",
    "    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);\n"
    "#ifdef __ANDROID__\n"
    "    {\n"
    "        SDL_Rect body = {0, 480, TOUCH_LOGICAL_W, TOUCH_LOGICAL_H - 480};\n"
    "        SDL_Rect seam = {0, 480, TOUCH_LOGICAL_W, 12};\n"
    "        SDL_Rect dpad_vertical = {112, 640, 66, 300};\n"
    "        SDL_Rect dpad_horizontal = {18, 757, 254, 66};\n"
    "        int slit;\n"
    "        SDL_SetRenderDrawColor(renderer, 194, 199, 181, 255);\n"
    "        SDL_RenderFillRect(renderer, &body);\n"
    "        SDL_SetRenderDrawColor(renderer, 74, 80, 75, 255);\n"
    "        SDL_RenderFillRect(renderer, &seam);\n"
    "        SDL_SetRenderDrawColor(renderer, 31, 34, 40, 145);\n"
    "        SDL_RenderFillRect(renderer, &dpad_vertical);\n"
    "        SDL_RenderFillRect(renderer, &dpad_horizontal);\n"
    "        SDL_SetRenderDrawColor(renderer, 90, 94, 87, 190);\n"
    "        for (slit = 0; slit < 6; ++slit)\n"
    "            SDL_RenderDrawLine(renderer, 466 + slit * 18, 1080,\n"
    "                               438 + slit * 18, 1160);\n"
    "    }\n"
    "#endif\n"
    "    alpha = (Uint8)clamp_int(touch->opacity * 255 / 100, 30, 255);",
)

# Android is portrait only. PC remains landscape/windowed 4:3.
replace_all(
    "android/app/src/main/AndroidManifest.xml",
    'android:screenOrientation="landscape"',
    'android:screenOrientation="portrait"',
    minimum=2,
)

# Version and artifacts must match the code being tested.
replace_once(
    "CMakeLists.txt",
    "project(tetris_nes_pc_port VERSION 0.21.0 LANGUAGES C)",
    "project(tetris_nes_pc_port VERSION 0.22.0 LANGUAGES C)",
)
replace_once("android/app/build.gradle", "versionCode 21", "versionCode 22")
replace_once(
    "android/app/build.gradle",
    "versionName '0.21-android'",
    "versionName '0.22-android'",
)
replace_all(".github/workflows/build.yml", "v0.21.0", "v0.22.0")
replace_all(".github/workflows/build.yml", "v0.21", "v0.22")
if "v0.21" in read(".github/workflows/android.yml"):
    replace_all(".github/workflows/android.yml", "v0.21", "v0.22")

# Permanent source-level regression check used by both workflows.
layout_test = '''#!/usr/bin/env python3
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
'''
write("tools/test_v022_layout_audio.py", layout_test)

for workflow in (".github/workflows/build.yml", ".github/workflows/android.yml"):
    content = read(workflow)
    marker = "          python tools/build_audio_cache.py --self-test\n"
    if marker not in content:
        raise RuntimeError(f"{workflow}: audio-cache self-test marker missing")
    content = content.replace(
        marker,
        marker + "          python tools/test_v022_layout_audio.py\n",
        1,
    )
    write(workflow, content)

# Minimal documentation update without claiming the audio is verified by ear.
readme = read("README.md")
readme = readme.replace("Versión **0.21**", "Versión **0.22**", 1)
readme += """

## Cambios de v0.22

- Los efectos del APU ya no descartan la música que estaba preparada en el
  ring buffer; mover o rotar una pieza no debe acelerar ni cortar la canción.
- El muestreo del APU ocurre durante todos los ciclos 6502, incluidos los
  ciclos del driver y los stalls DMC.
- PC conserva una ventana 4:3 exacta.
- Android usa orientación vertical con el juego arriba y controles grandes de
  estilo consola portátil abajo. Este diseño no se aplica al port de PC.
"""
write("README.md", readme)

print("Applied v0.22 audio continuity and platform layout patch")
