#!/usr/bin/env python3
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def write(path, text):
    target = ROOT / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text, encoding="utf-8")


def replace_once(path, old, new):
    text = read(path)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{path}: expected one match, found {count}: {old!r}")
    write(path, text.replace(old, new, 1))


def replace_optional(path, old, new):
    text = read(path)
    if old in text:
        write(path, text.replace(old, new))


# Exact NES horizontal autorepeat state must survive ARE/line-clear phases.
replace_once(
    "src/game.h",
    "    int das_counter;\n    int das_direction;\n    int soft_drop_counter;",
    "    int das_counter;\n    int das_direction;\n    uint8_t horizontal_buttons;\n    int soft_drop_counter;",
)

replace_once(
    "src/game.c",
    "    game->fall_counter = 0;\n    game->das_counter = 0;\n    game->das_direction = 0;\n    game->soft_drop_counter = 0;",
    "    game->fall_counter = 0;\n    /* The cartridge preserves horizontal DAS charge through ARE and clears. */\n    game->soft_drop_counter = 0;",
)

old_horizontal = '''static void update_horizontal(TetrisGame *game, const TetrisInput *input) {
    int direction = 0;
    if (input->down) {
        game->das_direction = 0;
        game->das_counter = 0;
        return;
    }
    if (input->left != input->right) direction = input->left ? -1 : 1;
    if (direction == 0) {
        game->das_direction = 0;
        game->das_counter = 0;
        return;
    }
    if (game->das_direction != direction) {
        game->das_direction = direction;
        game->das_counter = 0;
        (void)tetris_try_move(game, direction, 0);
        return;
    }

    ++game->das_counter;
    if (game->das_counter >= 16) {
        game->das_counter = 10;
        if (!tetris_try_move(game, direction, 0)) game->das_counter = 16;
    }
}
'''
new_horizontal = '''static uint8_t horizontal_input_mask(const TetrisInput *input) {
    uint8_t mask = 0u;
    if (!input) return 0u;
    if (input->left) mask |= 1u;
    if (input->right) mask |= 2u;
    return mask;
}

static void update_horizontal(TetrisGame *game, const TetrisInput *input) {
    const uint8_t held = horizontal_input_mask(input);
    const uint8_t newly_pressed = (uint8_t)(held & ~game->horizontal_buttons);
    int direction;

    /* Original shift_tetrimino returns without discharging DAS while DOWN is held. */
    if (input->down) return;

    /* The original checks RIGHT first, so RIGHT wins when both are held. */
    if ((held & 2u) != 0u) direction = 1;
    else if ((held & 1u) != 0u) direction = -1;
    else {
        game->das_direction = 0;
        return;
    }
    game->das_direction = direction;

    if (newly_pressed != 0u) {
        game->das_counter = 0;
    } else {
        ++game->das_counter;
        if (game->das_counter < 16) return;
        game->das_counter = 10;
    }

    if (!tetris_try_move(game, direction, 0)) {
        /* DAS_RESET=$10 on NTSC after a wall collision. */
        game->das_counter = 16;
    }
}
'''
replace_once("src/game.c", old_horizontal, new_horizontal)

replace_once(
    "src/game.c",
    '''static void tick_line_clear(TetrisGame *game) {
    ++game->phase_timer;
    game->clear_step = game->phase_timer / 4;
    if (game->clear_step > 4) game->clear_step = 4;
    if (game->phase_timer >= 20) finish_line_clear(game);
}
''',
    '''static void tick_line_clear(TetrisGame *game) {
    ++game->phase_timer;
    /* The PPU animation is keyed to the global NMI frame counter, not phase age. */
    if ((game->frame & 3) != 0) return;
    if (game->clear_step < 4) {
        ++game->clear_step;
        return;
    }
    finish_line_clear(game);
}
''',
)

replace_once(
    "src/game.c",
    "    if ((game->phase_timer & 3) == 0 && game->curtain_rows < TETRIS_BOARD_H) {",
    "    if ((game->frame & 3) == 0 && game->curtain_rows < TETRIS_BOARD_H) {",
)

replace_once(
    "src/game.c",
    '''void tetris_tick(TetrisGame *game, const TetrisInput *input) {
    if (input->restart_pressed &&''',
    '''void tetris_tick(TetrisGame *game, const TetrisInput *input) {
    const uint8_t held_horizontal = horizontal_input_mask(input);
    if (input->restart_pressed &&''',
)
replace_once(
    "src/game.c",
    '''        tetris_init_mode(game, seed, game->start_level,
                         game->mode, game->start_height);
        return;
''',
    '''        tetris_init_mode(game, seed, game->start_level,
                         game->mode, game->start_height);
        game->horizontal_buttons = held_horizontal;
        return;
''',
)
replace_once(
    "src/game.c",
    '''    advance_rng(game);
    if (game->paused || game->phase == TETRIS_PHASE_GAME_OVER ||
        game->phase == TETRIS_PHASE_COMPLETE) return;
''',
    '''    advance_rng(game);
    if (game->paused || game->phase == TETRIS_PHASE_GAME_OVER ||
        game->phase == TETRIS_PHASE_COMPLETE) {
        game->horizontal_buttons = held_horizontal;
        return;
    }
''',
)
replace_once(
    "src/game.c",
    '''        case TETRIS_PHASE_COMPLETE:
            break;
    }
}
''',
    '''        case TETRIS_PHASE_COMPLETE:
            break;
    }
    game->horizontal_buttons = held_horizontal;
}
''',
)

replace_once(
    "src/replay.c",
    "    hash_i32(&hash, game->das_direction);\n    hash_i32(&hash, game->soft_drop_counter);",
    "    hash_i32(&hash, game->das_direction);\n    hash_byte(&hash, game->horizontal_buttons);\n    hash_i32(&hash, game->soft_drop_counter);",
)

# Build/test integration and versioning.
replace_once(
    "CMakeLists.txt",
    "project(tetris_nes_pc_port VERSION 0.22.0 LANGUAGES C)",
    "project(tetris_nes_pc_port VERSION 0.23.0 LANGUAGES C)",
)
replace_once(
    "CMakeLists.txt",
    "    add_executable(tetris_audio_ring_tests tests/audio_ring_tests.c)\n",
    "    add_executable(tetris_audio_ring_tests tests/audio_ring_tests.c)\n    add_executable(tetris_timing_fidelity_tests tests/timing_fidelity_tests.c)\n",
)
replace_once(
    "CMakeLists.txt",
    "            tetris_highscores_tests tetris_playability_tests\n            tetris_audio_ring_tests)",
    "            tetris_highscores_tests tetris_playability_tests\n            tetris_audio_ring_tests tetris_timing_fidelity_tests)",
)
replace_once(
    "CMakeLists.txt",
    "    add_test(NAME audio_ring COMMAND tetris_audio_ring_tests)\n",
    "    add_test(NAME audio_ring COMMAND tetris_audio_ring_tests)\n    add_test(NAME timing_fidelity COMMAND tetris_timing_fidelity_tests)\n",
)
replace_once(
    "CMakeLists.txt",
    '''        add_test(NAME audio_cache_self_test
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/build_audio_cache.py --self-test)
''',
    '''        add_test(NAME audio_cache_self_test
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/build_audio_cache.py --self-test)
        add_test(NAME prg_verifier_self_test
                 COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/prg_verify.py --self-test)
''',
)
replace_once(
    "CMakeLists.txt",
    "    tools/apu_matrix.py tools/build_audio_cache.py tools/mesen_trace.lua\n",
    "    tools/apu_matrix.py tools/build_audio_cache.py tools/prg_verify.py tools/mesen_trace.lua\n",
)
replace_once(
    "CMakeLists.txt",
    "install(FILES README.md DESTINATION .)\n",
    "install(FILES README.md DESTINATION .)\ninstall(FILES tools/tetris_prg_manifest.json DESTINATION tools)\n",
)
replace_once(
    "CMakeLists.txt",
    "    docs/DECOMP_TOOLS.md docs/PLAYABILITY_VERIFICATION.md\n",
    "    docs/DECOMP_TOOLS.md docs/PLAYABILITY_VERIFICATION.md\n    docs/PRG_VERIFICATION.md\n",
)

replace_once("android/app/build.gradle", "versionCode 22", "versionCode 23")
replace_once("android/app/build.gradle", "versionName '0.22-android'", "versionName '0.23-android'")

# Upgrade every semantic label whose entry/signature was checked against the ROM.
symbol_path = ROOT / "tools/tetris_symbols.json"
symbol_data = json.loads(symbol_path.read_text(encoding="utf-8"))
symbols = symbol_data.setdefault("symbols", {})
symbols.update({
    "0x8161":"dispatch_game_mode",
    "0x8173":"process_player1",
    "0x8186":"process_player2",
    "0x819B":"dispatch_game_mode_state",
    "0x81B2":"dispatch_player1_play_state",
    "0x81CF":"player1_active_controls",
    "0x81D9":"dispatch_player2_play_state",
    "0x81F6":"player2_active_controls",
    "0x86DC":"game_state_init",
    "0x8776":"make_player1_active",
    "0x8792":"make_player2_active",
    "0x87AE":"save_player1_state",
    "0x87C8":"save_player2_state",
    "0x87DC":"init_type_b_playfield_gate",
    "0x87E3":"init_type_b_playfield",
    "0x87FC":"fill_type_b_row",
    "0x8824":"force_type_b_blank",
    "0x884A":"copy_type_b_playfield",
    "0x8875":"finish_type_b_init",
    "0x88AB":"rotate_tetrimino",
    "0x8914":"drop_tetrimino",
    "0x89AE":"shift_tetrimino",
    "0x8A0A":"stage_current_piece_sprite",
    "0x8A9C":"orientation_table",
    "0x988E":"spawn_next_piece",
    "0x993B":"tetrimino_type_from_orientation",
    "0x994E":"spawn_table",
    "0x9956":"spawn_orientation_from_orientation",
    "0x9969":"increment_piece_stat",
    "0x99A2":"lock_tetrimino",
    "0x9A11":"update_game_over_curtain",
    "0x9A6B":"check_completed_rows",
    "0x9B03":"receive_garbage",
    "0x9B53":"garbage_lines_by_clear_count",
    "0x9B58":"update_lines_and_statistics",
    "0x9CBF":"handle_game_over",
    "0x9D17":"update_music_speed",
    "0x9D51":"poll_controller_buttons",
    "0x9DE8":"advance_demo_stream",
    "0x9DF6":"start_demo",
    "0x9E07":"set_music_track",
    "0x9E16":"check_reset_combo",
    "0x9E27":"vblank_then_state2",
    "0x9E2F":"unassign_orientation",
    "0x9E37":"increment_play_state"
})
old_ranges = symbol_data.setdefault("data_ranges", [])
old_ranges = [item for item in old_ranges if int(item["start"], 0) not in
              {0x898E, 0x8A9C, 0x97FE, 0x984C, 0x993B, 0x994E, 0x9956, 0x9B53, 0x9CA5}]
old_ranges.extend([
    {"start":"0x898E","length":30,"name":"ntsc_gravity_table"},
    {"start":"0x8A9C","length":228,"name":"orientation_table"},
    {"start":"0x97FE","length":10,"name":"line_clear_columns"},
    {"start":"0x984C","length":40,"name":"level_palette_table"},
    {"start":"0x993B","length":19,"name":"tetrimino_type_from_orientation"},
    {"start":"0x994E","length":8,"name":"spawn_table"},
    {"start":"0x9956","length":19,"name":"spawn_orientation_from_orientation"},
    {"start":"0x9B53","length":5,"name":"garbage_lines_by_clear_count"},
    {"start":"0x9CA5","length":10,"name":"score_values_bcd"}
])
symbol_data["data_ranges"] = sorted(old_ranges, key=lambda item: int(item["start"], 0))
symbol_path.write_text(json.dumps(symbol_data, indent=2) + "\n", encoding="utf-8")

# Documentation generated from verified, non-proprietary facts.
write("docs/PRG_VERIFICATION.md", '''# Verificación semántica del PRG 6502

La versión 0.23 añade `tools/prg_verify.py` y
`tools/tetris_prg_manifest.json`. El verificador trabaja únicamente con la ROM
legal proporcionada por el usuario y no exporta PRG, CHR, audio ni capturas.

## Qué comprueba

- CRC32 y SHA-256 del archivo, PRG y CHR;
- vectores NMI, RESET e IRQ;
- 53 firmas de entrada asociadas a rutinas con nombre semántico;
- 9 tablas completas, incluida la tabla de 19 orientaciones;
- 14 aristas JSR del flujo de control;
- gravedad NTSC, columnas de borrado, apariciones, basura y puntuación BCD.

En total se verifican 1,136 bytes mediante firmas o hashes de tablas. El hash
del PRG completo confirma además que se está analizando la revisión esperada,
pero no se contabiliza como 100% decompilado: conocer el hash no equivale a
comprender, etiquetar y reconstruir cada rutina.

## Uso

```bash
python tools/prg_verify.py "Tetris (USA).nes" --report prg-report.json
python tools/disassemble_prg.py "Tetris (USA).nes" --aggressive \\
  --symbols tools/tetris_symbols.json --report disassembly-report.json
```

El informe contiene solo hashes, cantidades y resultados. No contiene bytes de
la ROM.

## Reglas y timing corregidos en 0.23

- `DAS_DELAY=$0A` y `DAS_RESET=$10` para NTSC;
- la carga horizontal se conserva durante ARE y borrado de líneas;
- mantener DOWN no descarga el DAS horizontal;
- RIGHT tiene prioridad cuando LEFT y RIGHT están pulsados simultáneamente;
- un movimiento bloqueado carga el contador hasta `DAS_RESET`;
- borrado de líneas y cortina de derrota se sincronizan con el contador global
  de frames, como las rutinas `$977F` y `$9A11`.

## Límite de la cifra de progreso

Una firma correcta demuestra que una dirección corresponde a la revisión y a
la rutina documentada. No demuestra por sí sola equivalencia de todos sus
estados internos. Por eso la cifra de decompilación sigue siendo una estimación
conservadora y separada de la cobertura heurística del desensamblador.
''')

readme = read("README.md")
readme = readme.replace("Versión **0.22**", "Versión **0.23**", 1)
readme = readme.replace("se estima en **46%**", "se estima en **50%**")
readme = readme.replace("aproximadamente **54%**", "aproximadamente **50%**")
section = '''\n## Cambios de v0.23\n\n- La ROM legal puede verificarse contra 53 entradas de rutina, 9 tablas y 14\n  aristas directas del PRG mediante `tools/prg_verify.py`.\n- El mapa 6502 incorpora estados de juego, controles, Modo B, selección de\n  piezas, líneas, puntuación, música y finalización.\n- El DAS horizontal conserva su carga a través de ARE y borrado de líneas.\n- DOWN ya no descarga el DAS; RIGHT conserva la prioridad original y una\n  colisión lateral carga el contador a 16.\n- Borrado de líneas y cortina usan el frame global en lugar de un temporizador\n  relativo a la fase.\n- La suite añade regresiones específicas de reglas/timing y el autoverificador\n  del manifiesto PRG.\n\n'''
if "## Cambios de v0.23" not in readme:
    readme = readme.replace("## Legalidad\n", section + "## Legalidad\n")
write("README.md", readme)

status = read("docs/PORT_STATUS.md")
status = status.replace("## Resumen de v0.21", "## Resumen de v0.23", 1)
status = status.replace("| Fidelidad de reglas y timings principales | 89% |",
                        "| Fidelidad de reglas y timings principales | 92% |")
status = status.replace("| Decompilación etiquetada/verificada del PRG 6502 | 45% |",
                        "| Decompilación etiquetada/verificada del PRG 6502 | 52% |")
status = status.replace("| **Correspondencia reproducible con la ROM** | **46%** |",
                        "| **Correspondencia reproducible con la ROM** | **50%** |")
status = status.replace("**Correspondencia reproducible con la ROM**: 46% terminado",
                        "**Correspondencia reproducible con la ROM**: 50% terminado")
status = status.replace("aproximadamente 54% pendiente", "aproximadamente 50% pendiente")
if "## Evidencia añadida en v0.23" not in status:
    status += '''\n## Evidencia añadida en v0.23\n\n- 53 firmas de rutinas enlazadas a nombres semánticos.\n- 9 tablas completas y 14 llamadas directas verificadas.\n- 1,136 bytes comprobados por firmas o hashes de tablas.\n- Hash exacto del PRG completo, sin contabilizarlo falsamente como decompilado.\n- Regresiones de DAS, prioridad simultánea, pared, ARE, línea y cortina.\n\nLas cifras suben solo por evidencia reproducible. Todavía falta traducir y\ncomparar más estados del bucle principal, menús, PPU, puntuación BCD y finales.\n'''
write("docs/PORT_STATUS.md", status)

rom_map = read("docs/ROM_MAP.md")
if "## Verificación ampliada v0.23" not in rom_map:
    rom_map += '''\n## Verificación ampliada v0.23\n\nEl manifiesto verifica 53 entradas de rutina. Entre las nuevas familias están:\n\n- `$8161-$81FC`: dispatch de modo, estado y controles de ambos jugadores;\n- `$86DC-$8875`: inicialización y generación completa del campo B-Type;\n- `$88AB/$8914/$89AE`: rotación, caída y desplazamiento/DAS;\n- `$988E-$9969`: aparición, selección aleatoria y estadísticas de piezas;\n- `$99A2-$9B58`: bloqueo, cortina, filas completas, basura, líneas y score;\n- `$9CBF-$9E37`: derrota, allegro, mando, demo y estados auxiliares.\n\nLa antigua región combinada de `$994E` se divide ahora en tres tablas:\n`tetrimino_type_from_orientation` (`$993B`, 19 bytes), `spawn_table`\n(`$994E`, 8 bytes) y `spawn_orientation_from_orientation` (`$9956`, 19 bytes).\nTambién se verifica `orientation_table` (`$8A9C`, 228 bytes) y\n`garbage_lines_by_clear_count` (`$9B53`, 5 bytes).\n'''
write("docs/ROM_MAP.md", rom_map)

for source in (ROOT / "src").glob("*.inc"):
    text = source.read_text(encoding="utf-8")
    updated = text.replace("Tetris NES PC Port v0.22", "Tetris NES PC Port v0.23")
    if updated != text:
        source.write_text(updated, encoding="utf-8")

# Remove the one-time patcher from the resulting source commit.
Path(__file__).unlink()
print("Applied v0.23 PRG verification and NES timing patch")
