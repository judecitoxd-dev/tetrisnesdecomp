#ifndef TETRIS_TOUCH_CONTROLS_H
#define TETRIS_TOUCH_CONTROLS_H

#include "settings.h"

#ifdef TETRIS_TOUCH_ACTION_COUNT
#undef TETRIS_TOUCH_ACTION_COUNT
#endif

#include <SDL.h>
#include <stdbool.h>

typedef enum TetrisTouchAction {
    TETRIS_TOUCH_NONE = -1,
    TETRIS_TOUCH_UP = 0,
    TETRIS_TOUCH_DOWN,
    TETRIS_TOUCH_LEFT,
    TETRIS_TOUCH_RIGHT,
    TETRIS_TOUCH_A,
    TETRIS_TOUCH_B,
    TETRIS_TOUCH_DROP,
    TETRIS_TOUCH_START,
    TETRIS_TOUCH_SELECT,
    TETRIS_TOUCH_ROM,
    TETRIS_TOUCH_EDIT,
    TETRIS_TOUCH_ACTION_COUNT
} TetrisTouchAction;

typedef struct TetrisTouchEvent {
    TetrisTouchAction action;
    bool pressed;
} TetrisTouchEvent;

typedef struct TetrisTouchControls {
    bool enabled;
    bool editing;
    bool dirty;
    int opacity;
    int scale;
    int x[TETRIS_TOUCH_ACTION_COUNT];
    int y[TETRIS_TOUCH_ACTION_COUNT];
    SDL_FingerID owner[TETRIS_TOUCH_ACTION_COUNT];
    bool held[TETRIS_TOUCH_ACTION_COUNT];
    int dragging;
    SDL_FingerID drag_finger;
} TetrisTouchControls;

void tetris_touch_init(TetrisTouchControls *touch,
                       const TetrisSettings *settings, bool enabled);
void tetris_touch_store_settings(const TetrisTouchControls *touch,
                                 TetrisSettings *settings);
bool tetris_touch_handle_event(TetrisTouchControls *touch,
                               const SDL_Event *event,
                               SDL_Window *window,
                               SDL_Renderer *renderer,
                               TetrisTouchEvent *result);
void tetris_touch_render(SDL_Renderer *renderer, SDL_Texture *font,
                         const TetrisTouchControls *touch,
                         bool controller_connected);
void tetris_touch_release_all(TetrisTouchControls *touch);

#endif
