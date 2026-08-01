    assert(game.start_level == 7);
    assert(game.start_height == 4);
    assert(game.lines == 25);
    assert(game.phase == TETRIS_PHASE_ACTIVE);
}


static void test_rom_level_palette_lookup(void) {
    uint8_t prg[0x184c + 40] = {0};
    uint8_t colors[4] = {0};
    NesRom rom = {0};
    rom.prg = prg;
    rom.prg_size = sizeof(prg);
    prg[0x184c + 3 * 4 + 0] = 0x0f;
    prg[0x184c + 3 * 4 + 1] = 0x30;
    prg[0x184c + 3 * 4 + 2] = 0x21;
    prg[0x184c + 3 * 4 + 3] = 0x12;
    assert(nes_rom_level_palette(&rom, 3, colors));
    assert(colors[0] == 0x0f && colors[1] == 0x30);
    assert(colors[2] == 0x21 && colors[3] == 0x12);
    assert(nes_rom_level_palette(&rom, 13, colors));
    assert(colors[2] == 0x21);
    rom.prg_size = 10;
    assert(!nes_rom_level_palette(&rom, 0, colors));
}

static void test_high_scores_sort_and_round_trip(void) {
    const char *path = "tetris_scores_test.tmp";
    TetrisHighScores scores;
    TetrisHighScores loaded;
    remove(path);
    tetris_high_scores_init(&scores);
    assert(tetris_high_scores_rank(&scores, TETRIS_MODE_A, 100) == 0);
    assert(tetris_high_scores_submit(&scores, TETRIS_MODE_A, "AAA", 100, 1, 0));
    assert(tetris_high_scores_submit(&scores, TETRIS_MODE_A, "BBB", 500, 5, 0));
    assert(tetris_high_scores_submit(&scores, TETRIS_MODE_A, "CCC", 300, 3, 0));
    assert(tetris_high_scores_submit(&scores, TETRIS_MODE_A, "DDD", 300, 4, 0));
    assert(tetris_high_scores_top(&scores, TETRIS_MODE_A)->score == 500);
    assert(tetris_high_scores_rank(&scores, TETRIS_MODE_A, 250) == -1);
    assert(strcmp(tetris_high_scores_top(&scores, TETRIS_MODE_A)->name, "BBB---") == 0);
    assert(tetris_high_scores_submit(&scores, TETRIS_MODE_B, "WIN", 14400, 9, 5));
    assert(tetris_high_scores_save(&scores, path));
    tetris_high_scores_init(&loaded);
    assert(tetris_high_scores_load(&loaded, path));
    assert(loaded.entries[0][0].score == 500);
    assert(loaded.entries[0][1].score == 300);
    assert(loaded.entries[0][2].score == 300);
    assert(loaded.entries[0][1].score == 300);
    assert(loaded.entries[1][0].score == 14400);
    assert(loaded.entries[1][0].height == 5);
    remove(path);
}

static void test_name_entry_editing(void) {
    TetrisNameEntry entry;
    tetris_name_entry_init(&entry, "abc");
    assert(strcmp(entry.text, "ABC---") == 0);
    tetris_name_entry_type(&entry, 'Z');
    assert(entry.text[0] == 'Z' && entry.cursor == 1);
    tetris_name_entry_cycle(&entry, 1);
    assert(entry.text[1] == 'C');
    tetris_name_entry_move(&entry, -2);
    assert(entry.cursor == 5);
    tetris_name_entry_type(&entry, '7');
    assert(strcmp(entry.text, "ZCC--7") == 0);
    tetris_name_entry_backspace(&entry);
    assert(entry.text[5] == '-' && entry.cursor == 4);
}

static void test_demo_controller_places_pieces(void) {
    TetrisGame game;
    TetrisDemoController demo;
    int peak_piece_count = 0;
    tetris_init_mode(&game, 0x19891101u, 5, TETRIS_MODE_A, 0);
    tetris_demo_reset(&demo);
    for (int frame = 0; frame < 5000 &&
         game.phase != TETRIS_PHASE_GAME_OVER; ++frame) {
        TetrisInput input = tetris_demo_next_input(&demo, &game);
        tetris_tick(&game, &input);
        {
            int total = 0;
            for (int piece = 0; piece < TETRIS_PIECE_COUNT; ++piece) {
                total += game.piece_count[piece];
            }
            if (total > peak_piece_count) peak_piece_count = total;
        }
    }
    assert(peak_piece_count >= 20);
}


static void test_settings_round_trip_and_sanitize(void) {
    const char *path = "tetris_settings_test.tmp";
    TetrisSettings settings;
    TetrisSettings loaded;
    remove(path);
    tetris_settings_init(&settings);
    settings.audio_enabled = false;
    settings.music_track = 2;
    settings.fullscreen = true;
    settings.integer_scale = false;
    settings.demo_enabled = false;
    settings.hard_drop_enabled = false;
    settings.show_next = false;
    settings.last_mode = TETRIS_MODE_B;
    settings.last_level = 18;
    settings.last_height = 5;
    settings.window_width = 1234;
    settings.window_height = 777;
    tetris_settings_set_rom_path(&settings, "/tmp/Tetris Test.nes\nignored");
    assert(tetris_settings_save(&settings, path));
    tetris_settings_init(&loaded);
    assert(tetris_settings_load(&loaded, path));
    assert(!loaded.audio_enabled && loaded.music_track == 2);
    assert(loaded.fullscreen && !loaded.integer_scale);
    assert(!loaded.demo_enabled && !loaded.hard_drop_enabled);
    assert(!loaded.show_next && loaded.last_mode == TETRIS_MODE_B);
    assert(loaded.last_level == 18 && loaded.last_height == 5);
    assert(loaded.window_width == 1234 && loaded.window_height == 777);
    assert(strcmp(loaded.rom_path, "/tmp/Tetris Test.nes ignored") == 0);
    loaded.music_track = 99;
    loaded.last_level = -20;
    loaded.last_height = 99;
    loaded.window_width = 10;
    tetris_settings_sanitize(&loaded);
    assert(loaded.music_track == 2);
    assert(loaded.last_level == 0 && loaded.last_height == 5);
    assert(loaded.window_width == 640);
    remove(path);
}

static void test_replay_round_trip_and_hash(void) {
    const char *path = "tetris_replay_test.ttr";
    TetrisGame original;
    TetrisGame reproduced;
    TetrisDemoController demo;
    TetrisReplay recording;
    TetrisReplay loaded;
    char error[256];
    uint32_t frame;
    remove(path);
    tetris_replay_init(&recording);
    tetris_replay_init(&loaded);
    tetris_init_mode(&original, 0x1234abcdU, 4, TETRIS_MODE_A, 0);
    original.show_next = false;
    tetris_demo_reset(&demo);
    assert(tetris_replay_begin(&recording, &original));
    for (frame = 0; frame < 1200 &&
         original.phase != TETRIS_PHASE_GAME_OVER; ++frame) {
        TetrisInput input = tetris_demo_next_input(&demo, &original);
        if (frame == 100) input.toggle_next_pressed = true;
        assert(tetris_replay_append(&recording, &input));
        tetris_tick(&original, &input);
        (void)tetris_consume_events(&original);
    }
    assert(recording.frame_count > 100);
    tetris_replay_finish(&recording, &original);
    assert(recording.final_hash == tetris_state_hash(&original));
    assert(tetris_replay_save(&recording, path));
    assert(tetris_replay_load(&loaded, path, error, sizeof(error)));
    assert(loaded.seed == 0x1234abcdU);
    assert(!loaded.initial_show_next);
    assert(loaded.frame_count == recording.frame_count);
