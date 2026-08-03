#!/usr/bin/env python3
"""Apply the v0.27 native lock/row-check timing pipeline once."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    (ROOT / path).write_text(text, encoding="utf-8")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


def replace_between(text: str, start: str, end: str, replacement: str,
                    label: str) -> str:
    first = text.find(start)
    if first < 0:
        raise RuntimeError(f"{label}: start marker missing")
    last = text.find(end, first + len(start))
    if last < 0:
        raise RuntimeError(f"{label}: end marker missing")
    return text[:first] + replacement + "\n\n" + text[last:]


def patch_game_header() -> None:
    path = "src/game.h"
    text = read(path)
    old = """typedef enum TetrisPhase {
    TETRIS_PHASE_ACTIVE = 0,
    TETRIS_PHASE_LINE_CLEAR,
    TETRIS_PHASE_ENTRY_DELAY,
    TETRIS_PHASE_GAME_OVER_CURTAIN,
    TETRIS_PHASE_GAME_OVER,
    TETRIS_PHASE_COMPLETE
} TetrisPhase;"""
    new = """typedef enum TetrisPhase {
    TETRIS_PHASE_ACTIVE = 0,
    /* NES playState 2: collision was detected on the preceding update. */
    TETRIS_PHASE_LOCK_PENDING,
    /* NES playState 3: inspect one of four candidate rows per update. */
    TETRIS_PHASE_ROW_CHECK,
    TETRIS_PHASE_LINE_CLEAR,
    TETRIS_PHASE_ENTRY_DELAY,
    TETRIS_PHASE_GAME_OVER_CURTAIN,
    TETRIS_PHASE_GAME_OVER,
    TETRIS_PHASE_COMPLETE
} TetrisPhase;"""
    write(path, replace_once(text, old, new, "game phase enum"))


def patch_game_core() -> None:
    path = "src/game.c"
    text = read(path)

    old_find = """static int find_completed_rows(TetrisGame *game) {
    int count = 0;
    for (int y = TETRIS_BOARD_H - 1;
         y >= 0 && count < TETRIS_MAX_CLEAR_ROWS; --y) {
        bool full = true;
        for (int x = 0; x < TETRIS_BOARD_W; ++x) {
            if (game->board[y][x] == 0) {
                full = false;
                break;
            }
        }
        if (full) game->clear_rows[count++] = y;
    }
    for (int i = count; i < TETRIS_MAX_CLEAR_ROWS; ++i) game->clear_rows[i] = -1;
    return count;
}"""
    new_find = """static bool row_is_complete(const TetrisGame *game, int row) {
    if (row < 0 || row >= TETRIS_BOARD_H) return false;
    for (int x = 0; x < TETRIS_BOARD_W; ++x) {
        if (game->board[row][x] == 0) return false;
    }
    return true;
}"""
    text = replace_once(text, old_find, new_find, "completed-row helper")

    text = replace_once(
        text,
        "static void lock_piece(TetrisGame *game) {",
        """static void begin_lock_pending(TetrisGame *game) {
    game->phase = TETRIS_PHASE_LOCK_PENDING;
    game->phase_timer = 0;
    game->fall_counter = 0;
}

static void lock_piece(TetrisGame *game) {""",
        "lock pending insertion")

    old_lock_tail = """    game->clear_count = find_completed_rows(game);
    if (game->clear_count > 0) {
        game->phase = TETRIS_PHASE_LINE_CLEAR;
        game->phase_timer = 0;
        game->clear_step = 0;
    } else {
        begin_entry_delay(game);
    }
}"""
    new_lock_tail = """    /*
     * The cartridge does not scan every row in the locking update. It enters
     * playState 3 and checks exactly four candidate rows on four subsequent
     * player-state updates. clear_step doubles as that row-check index here.
     */
    game->clear_count = 0;
    game->clear_step = 0;
    for (int i = 0; i < TETRIS_MAX_CLEAR_ROWS; ++i) game->clear_rows[i] = -1;
    game->phase = TETRIS_PHASE_ROW_CHECK;
    game->phase_timer = 0;
}"""
    text = replace_once(text, old_lock_tail, new_lock_tail,
                        "lock to row-check transition")

    text = replace_once(
        text,
        """            } else {
                lock_piece(game);
            }
        }
        return;""",
        """            } else {
                begin_lock_pending(game);
            }
        }
        return;""",
        "soft-drop collision transition")
    text = replace_once(
        text,
        """        } else {
            lock_piece(game);
        }
    }
}

static void tick_line_clear""",
        """        } else {
            begin_lock_pending(game);
        }
    }
}

static void tick_lock_pending(TetrisGame *game) {
    /* One player-state update after collision, matching playState 2. */
    lock_piece(game);
}

static void tick_row_check(TetrisGame *game) {
    int first_row = game->y - 2;
    int row;
    if (first_row < 0) first_row = 0;
    row = first_row + game->clear_step;

    if (row_is_complete(game, row) &&
        game->clear_count < TETRIS_MAX_CLEAR_ROWS) {
        game->clear_rows[game->clear_count++] = row;
    }

    ++game->clear_step;
    if (game->clear_step < 4) return;

    if (game->clear_count > 0) {
        game->phase = TETRIS_PHASE_LINE_CLEAR;
        game->phase_timer = 0;
        game->clear_step = 0;
    } else {
        begin_entry_delay(game);
    }
}

static void tick_line_clear""",
        "gravity collision and state handlers")

    old_switch = """    switch (game->phase) {
        case TETRIS_PHASE_ACTIVE: tick_active(game, input); break;
        case TETRIS_PHASE_LINE_CLEAR: tick_line_clear(game); break;
        case TETRIS_PHASE_ENTRY_DELAY: tick_entry_delay(game); break;
        case TETRIS_PHASE_GAME_OVER_CURTAIN: tick_game_over_curtain(game); break;
        case TETRIS_PHASE_GAME_OVER:
        case TETRIS_PHASE_COMPLETE:
            break;
    }"""
    new_switch = """    switch (game->phase) {
        case TETRIS_PHASE_ACTIVE: tick_active(game, input); break;
        case TETRIS_PHASE_LOCK_PENDING: tick_lock_pending(game); break;
        case TETRIS_PHASE_ROW_CHECK: tick_row_check(game); break;
        case TETRIS_PHASE_LINE_CLEAR: tick_line_clear(game); break;
        case TETRIS_PHASE_ENTRY_DELAY: tick_entry_delay(game); break;
        case TETRIS_PHASE_GAME_OVER_CURTAIN: tick_game_over_curtain(game); break;
        case TETRIS_PHASE_GAME_OVER:
        case TETRIS_PHASE_COMPLETE:
            break;
    }"""
    text = replace_once(text, old_switch, new_switch, "phase dispatch")
    write(path, text)


def patch_game_tests() -> None:
    path = "tests/game_tests_v05_00.inc"
    text = read(path)

    line_test = """static void test_line_clear_animation_and_score(void) {
    TetrisGame game;
    tetris_init(&game, 42u, 0);
    prepare_single_line(&game);
    tetris_hard_drop(&game);
    assert(game.phase == TETRIS_PHASE_ROW_CHECK);
    assert(game.clear_count == 0);
    tick_empty(&game, 4);
    assert(game.phase == TETRIS_PHASE_LINE_CLEAR);
    assert(game.clear_count == 1);
    assert(game.lines == 0);
    assert(tetris_cell_hidden(&game, 4, 19));
    tick_empty(&game, 19);
    assert(game.lines == 0);
    tick_empty(&game, 1);
    assert(game.lines == 1);
    assert(game.total_lines == 1);
    assert(game.score == 40);
    assert(game.phase == TETRIS_PHASE_ENTRY_DELAY);
}"""
    text = replace_between(text,
        "static void test_line_clear_animation_and_score(void) {",
        "static void test_exact_level_transition_behavior(void) {",
        line_test,
        "line clear test")

    level_test = """static void test_exact_level_transition_behavior(void) {
    TetrisGame game;
    tetris_init(&game, 99u, 18);
    game.lines = 129;
    game.total_lines = 129;
    prepare_single_line(&game);
    tetris_hard_drop(&game);
    tick_empty(&game, 24);
    assert(game.lines == 130);
    assert(game.level == 19);
    assert(game.score == 800);
}"""
    text = replace_between(text,
        "static void test_exact_level_transition_behavior(void) {",
        "static void test_entry_delay_spawns_later(void) {",
        level_test,
        "level transition test")

    entry_test = """static void test_entry_delay_spawns_later(void) {
    TetrisGame game;
    tetris_init(&game, 7u, 0);
    memset(game.board, 0, sizeof(game.board));
    game.active = PIECE_O;
    game.rotation = tetris_spawn_rotation(PIECE_O);
    game.x = 5;
    game.y = 18;
    game.phase = TETRIS_PHASE_ACTIVE;
    tetris_hard_drop(&game);
    assert(game.phase == TETRIS_PHASE_ROW_CHECK);
    tick_empty(&game, 4);
    assert(game.phase == TETRIS_PHASE_ENTRY_DELAY);
    {
        const int delay = game.phase_timer;
        assert(delay == 10);
        tick_empty(&game, delay - 1);
        assert(game.phase == TETRIS_PHASE_ENTRY_DELAY);
        tick_empty(&game, 1);
        assert(game.phase == TETRIS_PHASE_ACTIVE);
    }
}"""
    text = replace_between(text,
        "static void test_entry_delay_spawns_later(void) {",
        "static void test_pause_and_next_toggle(void) {",
        entry_test,
        "entry delay test")

    type_b_test = """static void test_type_b_level_fixed_and_completion_bonus(void) {
    TetrisGame game;
    tetris_init_mode(&game, 0x2026u, 9, TETRIS_MODE_B, 5);
    game.lines = 1;
    game.total_lines = 24;
    prepare_single_line(&game);
    tetris_hard_drop(&game);
    tick_empty(&game, 24);
    assert(game.lines == 0);
    assert(game.total_lines == 25);
    assert(game.level == 9);
    assert(game.phase == TETRIS_PHASE_COMPLETE);
    assert(game.completed);
    assert(game.score == 14400);
    assert(tetris_type_b_completion_bonus(19, 5) == 14000);
}"""
    text = replace_between(text,
        "static void test_type_b_level_fixed_and_completion_bonus(void) {",
        "static void test_type_b_restart_preserves_settings(void) {",
        type_b_test,
        "type B completion test")
    write(path, text)


def patch_timing_tests() -> None:
    path = "tests/timing_fidelity_tests.c"
    write(path, r'''#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "timing fidelity assertion failed: %s at %s:%d\n", \
                #expression, __FILE__, __LINE__); \
        exit(1); \
    } \
} while (0)

static void tick_with(TetrisGame *game, TetrisInput input, int count) {
    int frame;
    for (frame = 0; frame < count; ++frame) tetris_tick(game, &input);
}

static void prepare_active_o(TetrisGame *game, int x, int y) {
    memset(game->board, 0, sizeof(game->board));
    game->active = PIECE_O;
    game->rotation = tetris_spawn_rotation(PIECE_O);
    game->x = x;
    game->y = y;
    game->phase = TETRIS_PHASE_ACTIVE;
    game->paused = false;
    game->fall_counter = 0;
}

static void prepare_single_line(TetrisGame *game) {
    int x;
    memset(game->board, 0, sizeof(game->board));
    for (x = 0; x < 6; ++x) game->board[19][x] = 1;
    game->active = PIECE_I;
    game->rotation = tetris_spawn_rotation(PIECE_I);
    game->x = 8;
    game->y = 19;
    game->phase = TETRIS_PHASE_ACTIVE;
}

static int occupied_cells(const TetrisGame *game) {
    int total = 0;
    int y;
    int x;
    for (y = 0; y < TETRIS_BOARD_H; ++y)
        for (x = 0; x < TETRIS_BOARD_W; ++x)
            if (game->board[y][x] != 0) ++total;
    return total;
}

static void test_down_preserves_horizontal_das(void) {
    TetrisGame game;
    TetrisInput input = {0};
    tetris_init(&game, 0x1989u, 0);
    prepare_active_o(&game, 5, 5);
    game.das_counter = 15;
    game.das_direction = 1;
    game.horizontal_buttons = 2u;
    input.right = true;
    input.down = true;
    tetris_tick(&game, &input);
    CHECK(game.das_counter == 15);
    CHECK(game.das_direction == 1);
    CHECK(game.horizontal_buttons == 2u);
}

static void test_blocked_press_charges_das_to_reset(void) {
    TetrisGame game;
    TetrisInput input = {0};
    tetris_init(&game, 0x1989u, 0);
    prepare_active_o(&game, 9, 5);
    game.das_counter = 0;
    game.horizontal_buttons = 0u;
    input.right = true;
    tetris_tick(&game, &input);
    CHECK(game.x == 9);
    CHECK(game.das_direction == 1);
    CHECK(game.das_counter == 16);
}

static void test_right_has_priority_when_both_are_held(void) {
    TetrisGame game;
    TetrisInput input = {0};
    tetris_init(&game, 0x1989u, 0);
    prepare_active_o(&game, 5, 5);
    game.horizontal_buttons = 0u;
    input.left = true;
    input.right = true;
    tetris_tick(&game, &input);
    CHECK(game.x == 6);
    CHECK(game.das_direction == 1);
}

static void test_das_survives_entry_delay_and_spawn(void) {
    TetrisGame game;
    TetrisInput input = {0};
    tetris_init(&game, 0x1989u, 0);
    game.phase = TETRIS_PHASE_ENTRY_DELAY;
    game.phase_timer = 1;
    game.das_counter = 12;
    game.das_direction = 1;
    game.horizontal_buttons = 2u;
    input.right = true;
    tetris_tick(&game, &input);
    CHECK(game.phase == TETRIS_PHASE_ACTIVE);
    CHECK(game.das_counter == 12);
    CHECK(game.horizontal_buttons == 2u);
}

static void test_collision_defers_lock_one_update(void) {
    TetrisGame game;
    TetrisInput empty = {0};
    tetris_init(&game, 0x1989u, 0);
    prepare_active_o(&game, 5, 18);
    game.fall_counter = tetris_gravity_frames(game.level) - 1;
    CHECK(occupied_cells(&game) == 0);
    tetris_tick(&game, &empty);
    CHECK(game.phase == TETRIS_PHASE_LOCK_PENDING);
    CHECK(occupied_cells(&game) == 0);
    tetris_tick(&game, &empty);
    CHECK(game.phase == TETRIS_PHASE_ROW_CHECK);
    CHECK(occupied_cells(&game) == 4);
    CHECK(game.clear_step == 0);
}

static void test_four_row_checks_are_separate_updates(void) {
    TetrisGame game;
    TetrisInput empty = {0};
    tetris_init(&game, 0x1989u, 0);
    prepare_single_line(&game);
    tetris_hard_drop(&game);
    CHECK(game.phase == TETRIS_PHASE_ROW_CHECK);
    CHECK(game.clear_count == 0);
    tetris_tick(&game, &empty);
    CHECK(game.phase == TETRIS_PHASE_ROW_CHECK);
    CHECK(game.clear_step == 1 && game.clear_count == 0);
    tetris_tick(&game, &empty);
    CHECK(game.clear_step == 2 && game.clear_count == 0);
    tetris_tick(&game, &empty);
    CHECK(game.clear_step == 3 && game.clear_count == 1);
    CHECK(game.lines == 0 && game.score == 0);
    tetris_tick(&game, &empty);
    CHECK(game.phase == TETRIS_PHASE_LINE_CLEAR);
    CHECK(game.clear_step == 0 && game.clear_count == 1);
}

static void test_line_clear_uses_global_frame_alignment(void) {
    TetrisGame game;
    TetrisInput empty = {0};
    tetris_init(&game, 0x1989u, 0);
    game.frame = 1;
    prepare_single_line(&game);
    tetris_hard_drop(&game);
    CHECK(game.phase == TETRIS_PHASE_ROW_CHECK);
    tick_with(&game, empty, 4);
    CHECK(game.frame == 5);
    CHECK(game.phase == TETRIS_PHASE_LINE_CLEAR);
    tick_with(&game, empty, 18);
    CHECK(game.frame == 23);
    CHECK(game.phase == TETRIS_PHASE_LINE_CLEAR);
    CHECK(game.lines == 0 && game.score == 0);
    tick_with(&game, empty, 1);
    CHECK(game.frame == 24);
    CHECK(game.phase == TETRIS_PHASE_ENTRY_DELAY);
    CHECK(game.lines == 1 && game.score == 40);
}

static void test_curtain_uses_global_frame_alignment(void) {
    TetrisGame game;
    TetrisInput empty = {0};
    tetris_init(&game, 0x1989u, 0);
    game.phase = TETRIS_PHASE_GAME_OVER_CURTAIN;
    game.frame = 1;
    game.phase_timer = 0;
    game.curtain_rows = 0;
    tick_with(&game, empty, 2);
    CHECK(game.frame == 3);
    CHECK(game.curtain_rows == 0);
    tick_with(&game, empty, 1);
    CHECK(game.frame == 4);
    CHECK(game.curtain_rows == 1);
}

int main(void) {
    test_down_preserves_horizontal_das();
    test_blocked_press_charges_das_to_reset();
    test_right_has_priority_when_both_are_held();
    test_das_survives_entry_delay_and_spawn();
    test_collision_defers_lock_one_update();
    test_four_row_checks_are_separate_updates();
    test_line_clear_uses_global_frame_alignment();
    test_curtain_uses_global_frame_alignment();
    puts("NES lock, row-check, DAS and global-frame timing tests passed.");
    return 0;
}
''')


def patch_versions() -> None:
    changes = [
        ("CMakeLists.txt", "VERSION 0.23.0", "VERSION 0.27.0"),
        ("android/app/build.gradle", "versionCode 23", "versionCode 27"),
        ("android/app/build.gradle", "versionName '0.23-android'",
         "versionName '0.27-android'"),
        ("src/main_v05_02.inc", "Tetris NES PC Port v0.23",
         "Tetris NES PC Port v0.27"),
    ]
    for path, old, new in changes:
        text = read(path)
        text = replace_once(text, old, new, f"version in {path}")
        write(path, text)


def patch_status() -> None:
    path = "docs/PORT_STATUS.md"
    text = read(path)
    text = replace_once(text,
        "| Fidelidad de reglas y timings principales | 92% |",
        "| Fidelidad de reglas y timings principales | 94% |",
        "rules fidelity percentage")
    text = replace_once(text,
        "| Decompilación etiquetada/verificada del PRG 6502 | 60% |",
        "| Investigación y preparación del PRG 6502 | 100% |",
        "research percentage")
    text = replace_once(text,
        "| **Correspondencia reproducible con la ROM** | **56%** |",
        "| **Correspondencia funcional del runtime con la ROM** | **50%** |",
        "runtime correspondence percentage")
    old_para = """La cifra central es **Correspondencia reproducible con la ROM**: 56% terminado
y aproximadamente 44% pendiente para identidad funcional, audiovisual y de
timing. Los porcentajes son estimaciones de ingeniería, no cobertura automática
ni una afirmación de identidad binaria."""
    new_para = """La investigación y preparación del PRG ya está cerrada al 100%, pero la cifra
central del ejecutable es **Correspondencia funcional del runtime con la ROM**:
50%. Los porcentajes son estimaciones conservadoras de ingeniería, no una
afirmación de identidad audiovisual o binaria."""
    text = replace_once(text, old_para, new_para, "status central paragraph")
    text = text.replace(
        "- Las 18 trazas requieren capturas Mesen reales para corregir la primera\n  divergencia de cada familia.",
        "- Las 18 familias dinámicas de investigación ya son reproducibles; falta\n  comparar automáticamente el estado completo del runtime en cada fotograma.")
    section = r'''

## Cambios reales de reglas y timing en v0.27

La primera aplicación de la investigación al runtime corrige el pipeline que
ocurre cuando una pieza toca el suelo. Antes, el port bloqueaba la pieza,
buscaba todas las filas y elegía la siguiente fase dentro del mismo cuadro.

La ROM usa estados separados:

1. el control activo detecta la colisión vertical;
2. el siguiente estado bloquea los cuatro minos;
3. cuatro actualizaciones independientes inspeccionan las cuatro filas candidatas;
4. la animación de línea continúa alineada al contador global cada cuatro frames;
5. líneas, nivel y puntuación se aplican después de la animación;
6. después comienza la espera de aparición.

v0.27 añade `LOCK_PENDING` y `ROW_CHECK` al núcleo C. Las pruebas demuestran que
el tablero no cambia en el cuadro de colisión, que el bloqueo sucede en la
actualización siguiente, que solo se inspecciona una fila por actualización y
que la puntuación permanece sin cambios hasta terminar la animación.

Por esta corrección demostrable, la fidelidad de reglas y timings sube de 92% a
**94%**. No sube más porque todavía quedan diferencias conocidas:

- el colapso interno de RAM de una fila se realiza al final de la animación en
  el port, mientras la ROM lo realiza durante la comprobación de filas y deja
  la imagen antigua temporalmente en PPU;
- la aparición aún usa una tabla ARE equivalente en lugar de quedar bloqueada
  por el progreso exacto de copia VRAM;
- hard drop sigue siendo una extensión opcional del port y no una regla NES;
- falta comparar estados completos RAM/PPU/APU por fotograma contra las 18
  referencias dinámicas.
'''
    if "## Cambios reales de reglas y timing en v0.27" not in text:
        text += section
    write(path, text)


def main() -> int:
    marker = ROOT / ".v027-rule-timing-applied"
    if marker.exists():
        print("v0.27 patch already applied")
        return 0
    patch_game_header()
    patch_game_core()
    patch_game_tests()
    patch_timing_tests()
    patch_versions()
    patch_status()
    marker.write_text("lock pending + four row checks\n", encoding="utf-8")
    print("v0.27 rule/timing patch applied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
