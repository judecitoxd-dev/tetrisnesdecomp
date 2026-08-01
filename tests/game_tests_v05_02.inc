    tetris_init_mode(&reproduced, loaded.seed, loaded.start_level,
                     loaded.mode, loaded.start_height);
    reproduced.show_next = loaded.initial_show_next;
    for (frame = 0; frame < loaded.frame_count; ++frame) {
        TetrisInput input = tetris_replay_input(&loaded, frame);
        tetris_tick(&reproduced, &input);
        (void)tetris_consume_events(&reproduced);
    }
    assert(tetris_state_hash(&reproduced) == loaded.final_hash);
    assert(tetris_state_hash(&reproduced) == tetris_state_hash(&original));
    {
        FILE *file = fopen(path, "ab");
        assert(file != NULL);
        assert(fputc(0x55, file) != EOF);
        assert(fclose(file) == 0);
        assert(!tetris_replay_load(&loaded, path, error, sizeof(error)));
        assert(strstr(error, "trailing") != NULL);
    }
    tetris_replay_free(&recording);
    tetris_replay_free(&loaded);
    remove(path);
}

int main(void) {
    test_spawn_and_original_orientation();
    test_gravity_table();
    test_level_transition_table();
    test_entry_delay_table();
    test_line_clear_animation_and_score();
    test_exact_level_transition_behavior();
    test_entry_delay_spawns_later();
    test_pause_and_next_toggle();
    test_rng_is_deterministic();
    test_type_b_height_table_and_garbage();
    test_type_b_level_fixed_and_completion_bonus();
    test_type_b_restart_preserves_settings();
    test_rom_level_palette_lookup();
    test_high_scores_sort_and_round_trip();
    test_name_entry_editing();
    test_demo_controller_places_pieces();
    test_settings_round_trip_and_sanitize();
    test_replay_round_trip_and_hash();
    puts("All gameplay, replay, settings, demo and persistence tests passed.");
    return 0;
}
