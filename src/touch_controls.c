#include "touch_controls.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define TOUCH_LOGICAL_W 640
#ifdef __ANDROID__
#define TOUCH_LOGICAL_H 1280
#else
#define TOUCH_LOGICAL_H 480
#endif
#define NO_FINGER ((SDL_FingerID)-1)

static const char *const LABELS[TETRIS_TOUCH_ACTION_COUNT] = {
    "UP", "DOWN", "LEFT", "RIGHT", "A", "B", "DROP",
    "START", "SEL", "ROM", "EDIT"
};

static int clamp_int(int value, int low, int high) {
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static void default_positions(int x[TETRIS_TOUCH_ACTION_COUNT],
                              int y[TETRIS_TOUCH_ACTION_COUNT]) {
#ifdef __ANDROID__
    static const int defaults[TETRIS_TOUCH_ACTION_COUNT][2] = {
        {145, 690}, {145, 890}, {55, 790}, {235, 790},
        {535, 710}, {435, 820}, {535, 950},
        {380, 1150}, {260, 1150}, {70, 545}, {570, 545}
    };
#else
    static const int defaults[TETRIS_TOUCH_ACTION_COUNT][2] = {
        {105, 326}, {105, 430}, {53, 378}, {157, 378},
        {566, 330}, {494, 384}, {566, 428},
        {370, 447}, {278, 447}, {48, 28}, {590, 28}
    };
#endif
    int i;
    for (i = 0; i < TETRIS_TOUCH_ACTION_COUNT; ++i) {
        x[i] = defaults[i][0];
        y[i] = defaults[i][1];
    }
}

static int base_radius(TetrisTouchAction action) {
#ifdef __ANDROID__
    switch (action) {
        case TETRIS_TOUCH_START:
        case TETRIS_TOUCH_SELECT:
            return 34;
        case TETRIS_TOUCH_ROM:
        case TETRIS_TOUCH_EDIT:
            return 28;
        case TETRIS_TOUCH_DROP:
            return 48;
        case TETRIS_TOUCH_A:
        case TETRIS_TOUCH_B:
            return 54;
        default:
            return 50;
    }
#else
    switch (action) {
        case TETRIS_TOUCH_START:
        case TETRIS_TOUCH_SELECT:
            return 30;
        case TETRIS_TOUCH_ROM:
        case TETRIS_TOUCH_EDIT:
            return 26;
        case TETRIS_TOUCH_DROP:
            return 31;
        default:
            return 34;
    }
#endif
}

static int radius_for(const TetrisTouchControls *touch,
                      TetrisTouchAction action) {
#ifdef __ANDROID__
    return clamp_int(base_radius(action) * touch->scale / 100, 22, 76);
#else
    return clamp_int(base_radius(action) * touch->scale / 100, 18, 62);
#endif
}

static bool point_inside(const TetrisTouchControls *touch,
                         TetrisTouchAction action, float x, float y) {
    const float dx = x - (float)touch->x[action];
    const float dy = y - (float)touch->y[action];
    const float radius = (float)radius_for(touch, action);
    if (action == TETRIS_TOUCH_START || action == TETRIS_TOUCH_SELECT ||
        action == TETRIS_TOUCH_ROM || action == TETRIS_TOUCH_EDIT) {
        return fabsf(dx) <= radius * 1.45f && fabsf(dy) <= radius * 0.62f;
    }
    return dx * dx + dy * dy <= radius * radius;
}

static int action_at(const TetrisTouchControls *touch, float x, float y,
                     bool include_edit, bool include_rom) {
    int action;
    if (include_edit && point_inside(touch, TETRIS_TOUCH_EDIT, x, y))
        return TETRIS_TOUCH_EDIT;
    if (include_rom && point_inside(touch, TETRIS_TOUCH_ROM, x, y))
        return TETRIS_TOUCH_ROM;
    for (action = 0; action < TETRIS_TOUCH_ACTION_COUNT; ++action) {
        if (action == TETRIS_TOUCH_EDIT || action == TETRIS_TOUCH_ROM) continue;
        if (point_inside(touch, (TetrisTouchAction)action, x, y)) return action;
    }
    return -1;
}

static bool logical_point(const SDL_TouchFingerEvent *finger,
                          SDL_Window *window, SDL_Renderer *renderer,
                          float *x, float *y) {
    int width = 0;
    int height = 0;
    float window_x;
    float window_y;
    SDL_GetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0) return false;
    window_x = finger->x * (float)width;
    window_y = finger->y * (float)height;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    SDL_RenderWindowToLogical(renderer, (int)window_x, (int)window_y, x, y);
#else
    *x = window_x * TOUCH_LOGICAL_W / (float)width;
    *y = window_y * TOUCH_LOGICAL_H / (float)height;
#endif
    return true;
}

static bool emit(TetrisTouchEvent *result, TetrisTouchAction action,
                 bool pressed) {
    if (!result) return false;
    result->action = action;
    result->pressed = pressed;
    return true;
}

static bool release_owned(TetrisTouchControls *touch, SDL_FingerID finger,
                          TetrisTouchEvent *result) {
    int action;
    for (action = 0; action < TETRIS_TOUCH_ACTION_COUNT; ++action) {
        if (touch->owner[action] == finger && touch->held[action]) {
            touch->owner[action] = NO_FINGER;
            touch->held[action] = false;
            return emit(result, (TetrisTouchAction)action, false);
        }
    }
    return false;
}

static void cycle_edit_value(TetrisTouchControls *touch,
                             TetrisTouchAction action) {
    if (action == TETRIS_TOUCH_SELECT) {
        if (touch->opacity >= 100) touch->opacity = 30;
        else touch->opacity += 10;
    } else if (action == TETRIS_TOUCH_START) {
        if (touch->scale >= 150) touch->scale = 70;
        else touch->scale += 10;
    }
    touch->dirty = true;
}

void tetris_touch_init(TetrisTouchControls *touch,
                       const TetrisSettings *settings, bool enabled) {
    int i;
    if (!touch) return;
    memset(touch, 0, sizeof(*touch));
    touch->enabled = enabled;
    touch->dragging = -1;
    touch->drag_finger = NO_FINGER;
    for (i = 0; i < TETRIS_TOUCH_ACTION_COUNT; ++i)
        touch->owner[i] = NO_FINGER;
    default_positions(touch->x, touch->y);
#ifdef __ANDROID__
    touch->opacity = 86;
    touch->scale = 110;
#else
    touch->opacity = 58;
    touch->scale = 100;
#endif
    if (settings) {
        touch->opacity = settings->touch_opacity;
        touch->scale = settings->touch_scale;
        for (i = 0; i < TETRIS_TOUCH_ACTION_COUNT; ++i) {
            if (settings->touch_x[i] >= 0 && settings->touch_x[i] <= 1000 &&
                settings->touch_y[i] >= 0 && settings->touch_y[i] <= 1000) {
                touch->x[i] = settings->touch_x[i] * TOUCH_LOGICAL_W / 1000;
                touch->y[i] = settings->touch_y[i] * TOUCH_LOGICAL_H / 1000;
            }
        }
    }
    touch->opacity = clamp_int(touch->opacity, 20, 100);
    touch->scale = clamp_int(touch->scale, 60, 160);
#ifdef __ANDROID__
    if (touch->opacity < 68) touch->opacity = 68;
    if (touch->scale < 100) touch->scale = 100;
#endif
}

void tetris_touch_store_settings(const TetrisTouchControls *touch,
                                 TetrisSettings *settings) {
    int i;
    if (!touch || !settings) return;
    settings->touch_opacity = touch->opacity;
    settings->touch_scale = touch->scale;
    for (i = 0; i < TETRIS_TOUCH_ACTION_COUNT; ++i) {
        settings->touch_x[i] = clamp_int(touch->x[i] * 1000 / TOUCH_LOGICAL_W,
                                         0, 1000);
        settings->touch_y[i] = clamp_int(touch->y[i] * 1000 / TOUCH_LOGICAL_H,
                                         0, 1000);
    }
}

bool tetris_touch_handle_event(TetrisTouchControls *touch,
                               const SDL_Event *event,
                               SDL_Window *window,
                               SDL_Renderer *renderer,
                               TetrisTouchEvent *result) {
    float x;
    float y;
    int action;
    if (result) {
        result->action = TETRIS_TOUCH_NONE;
        result->pressed = false;
    }
    if (!touch || !touch->enabled || !event || !window || !renderer) return false;
    if (event->type != SDL_FINGERDOWN && event->type != SDL_FINGERMOTION &&
        event->type != SDL_FINGERUP) return false;
    if (!logical_point(&event->tfinger, window, renderer, &x, &y)) return false;

    if (event->type == SDL_FINGERDOWN) {
        action = action_at(touch, x, y, true, true);
        if (action == TETRIS_TOUCH_EDIT) {
            touch->editing = !touch->editing;
            touch->dirty = true;
            tetris_touch_release_all(touch);
            return true;
        }
        if (action == TETRIS_TOUCH_ROM && !touch->editing)
            return emit(result, TETRIS_TOUCH_ROM, true);
        if (touch->editing) {
            if (action == TETRIS_TOUCH_SELECT || action == TETRIS_TOUCH_START) {
                cycle_edit_value(touch, (TetrisTouchAction)action);
                return true;
            }
            if (action >= 0 && action != TETRIS_TOUCH_EDIT) {
                touch->dragging = action;
                touch->drag_finger = event->tfinger.fingerId;
                return true;
            }
            return true;
        }
        if (action >= 0 && action < TETRIS_TOUCH_ACTION_COUNT) {
            touch->owner[action] = event->tfinger.fingerId;
            touch->held[action] = true;
            return emit(result, (TetrisTouchAction)action, true);
        }
        return false;
    }

    if (event->type == SDL_FINGERMOTION) {
        if (touch->editing && touch->dragging >= 0 &&
            touch->drag_finger == event->tfinger.fingerId) {
            const int radius = radius_for(touch,
                (TetrisTouchAction)touch->dragging);
            touch->x[touch->dragging] = clamp_int((int)x, radius, TOUCH_LOGICAL_W - radius);
            touch->y[touch->dragging] = clamp_int((int)y, radius, TOUCH_LOGICAL_H - radius);
            touch->dirty = true;
            return true;
        }
        if (!touch->editing) {
            int old_action = -1;
            int i;
            for (i = 0; i < TETRIS_TOUCH_ACTION_COUNT; ++i) {
                if (touch->owner[i] == event->tfinger.fingerId && touch->held[i]) {
                    old_action = i;
                    break;
                }
            }
            action = action_at(touch, x, y, false, false);
            if (old_action >= 0 && action != old_action) {
                touch->owner[old_action] = NO_FINGER;
                touch->held[old_action] = false;
                return emit(result, (TetrisTouchAction)old_action, false);
            }
        }
        return false;
    }

    if (touch->editing && touch->drag_finger == event->tfinger.fingerId) {
        touch->dragging = -1;
        touch->drag_finger = NO_FINGER;
        return true;
    }
    return release_owned(touch, event->tfinger.fingerId, result);
}

static int font_tile(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return 10 + c - 'A';
    if (c == '-') return 36;
    return -1;
}

static void draw_label(SDL_Renderer *renderer, SDL_Texture *font,
                       int center_x, int center_y, int scale,
                       const char *text, Uint8 alpha) {
    int width;
    int x;
    const char *p;
    if (!font || !text) return;
    width = (int)strlen(text) * 8 * scale;
    x = center_x - width / 2;
    SDL_SetTextureAlphaMod(font, alpha);
    for (p = text; *p; ++p) {
        const int tile = font_tile(*p);
        if (tile >= 0) {
            SDL_Rect src = {(tile % 16) * 8, (tile / 16) * 8, 8, 8};
            SDL_Rect dst = {x, center_y - 4 * scale, 8 * scale, 8 * scale};
            SDL_RenderCopy(renderer, font, &src, &dst);
        }
        x += 8 * scale;
    }
    SDL_SetTextureAlphaMod(font, 255);
}

static void fill_circle(SDL_Renderer *renderer, int cx, int cy, int radius) {
    int y;
    for (y = -radius; y <= radius; ++y) {
        const int half = (int)sqrt((double)(radius * radius - y * y));
        SDL_RenderDrawLine(renderer, cx - half, cy + y, cx + half, cy + y);
    }
}

static void draw_control(SDL_Renderer *renderer, SDL_Texture *font,
                         const TetrisTouchControls *touch,
                         TetrisTouchAction action, Uint8 alpha) {
    const int radius = radius_for(touch, action);
    const int x = touch->x[action];
    const int y = touch->y[action];
    const bool wide = action == TETRIS_TOUCH_START ||
                      action == TETRIS_TOUCH_SELECT ||
                      action == TETRIS_TOUCH_ROM ||
                      action == TETRIS_TOUCH_EDIT;
    const Uint8 fill_alpha = touch->held[action] ? (Uint8)clamp_int(alpha + 55, 0, 255)
                                                  : (Uint8)(alpha / 2);
#ifdef __ANDROID__
    if (action == TETRIS_TOUCH_A || action == TETRIS_TOUCH_B ||
        action == TETRIS_TOUCH_DROP)
        SDL_SetRenderDrawColor(renderer, 142, 35, 67, fill_alpha);
    else
        SDL_SetRenderDrawColor(renderer, 43, 46, 52, fill_alpha);
#else
    SDL_SetRenderDrawColor(renderer, 245, 245, 245, fill_alpha);
#endif
    if (wide) {
        SDL_Rect box = {x - radius * 3 / 2, y - radius * 2 / 3,
                        radius * 3, radius * 4 / 3};
        SDL_RenderFillRect(renderer, &box);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
        SDL_RenderDrawRect(renderer, &box);
    } else {
        fill_circle(renderer, x, y, radius);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
        for (int ring = 0; ring < 2; ++ring) {
            const int rr = radius - ring;
            int px = x + rr;
            int py = y;
            for (int degree = 5; degree <= 360; degree += 5) {
                const double radians = degree * 3.14159265358979323846 / 180.0;
                const int nx = x + (int)(cos(radians) * rr);
                const int ny = y + (int)(sin(radians) * rr);
                SDL_RenderDrawLine(renderer, px, py, nx, ny);
                px = nx;
                py = ny;
            }
        }
    }
    draw_label(renderer, font, x, y, action == TETRIS_TOUCH_START ? 1 : 1,
               LABELS[action], alpha);
}

void tetris_touch_render(SDL_Renderer *renderer, SDL_Texture *font,
                         const TetrisTouchControls *touch,
                         bool controller_connected) {
    int action;
    Uint8 alpha;
    if (!renderer || !touch || !touch->enabled || controller_connected) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
#ifdef __ANDROID__
    {
        SDL_Rect body = {0, 480, TOUCH_LOGICAL_W, TOUCH_LOGICAL_H - 480};
        SDL_Rect seam = {0, 480, TOUCH_LOGICAL_W, 12};
        SDL_Rect dpad_vertical = {112, 640, 66, 300};
        SDL_Rect dpad_horizontal = {18, 757, 254, 66};
        int slit;
        SDL_SetRenderDrawColor(renderer, 194, 199, 181, 255);
        SDL_RenderFillRect(renderer, &body);
        SDL_SetRenderDrawColor(renderer, 74, 80, 75, 255);
        SDL_RenderFillRect(renderer, &seam);
        SDL_SetRenderDrawColor(renderer, 31, 34, 40, 145);
        SDL_RenderFillRect(renderer, &dpad_vertical);
        SDL_RenderFillRect(renderer, &dpad_horizontal);
        SDL_SetRenderDrawColor(renderer, 90, 94, 87, 190);
        for (slit = 0; slit < 6; ++slit)
            SDL_RenderDrawLine(renderer, 466 + slit * 18, 1080,
                               438 + slit * 18, 1160);
    }
#endif
    alpha = (Uint8)clamp_int(touch->opacity * 255 / 100, 30, 255);
    for (action = 0; action < TETRIS_TOUCH_ACTION_COUNT; ++action) {
        if (!touch->editing && action == TETRIS_TOUCH_EDIT) {
            draw_control(renderer, font, touch, (TetrisTouchAction)action,
                         (Uint8)clamp_int(alpha / 2, 24, 110));
            continue;
        }
        draw_control(renderer, font, touch, (TetrisTouchAction)action, alpha);
    }
    if (touch->editing) {
        char status[64];
        snprintf(status, sizeof(status), "EDIT SIZE %d OP %d",
                 touch->scale, touch->opacity);
        draw_label(renderer, font, TOUCH_LOGICAL_W / 2, 20, 1,
                   status, 255);
    }
}

void tetris_touch_release_all(TetrisTouchControls *touch) {
    int i;
    if (!touch) return;
    for (i = 0; i < TETRIS_TOUCH_ACTION_COUNT; ++i) {
        touch->held[i] = false;
        touch->owner[i] = NO_FINGER;
    }
    touch->dragging = -1;
    touch->drag_finger = NO_FINGER;
}
