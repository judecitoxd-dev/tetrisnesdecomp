#ifndef TETRIS_MENU_MUSIC_CURSOR_H
#define TETRIS_MENU_MUSIC_CURSOR_H

#include "app.h"

void render_type_music_cursor_overlay(SDL_Renderer *renderer,
                                      SDL_Texture *font,
                                      AppScreen screen,
                                      const TetrisSettings *settings);

#endif
