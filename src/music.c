Music main_bgm;
Music menu_bgm;
int music_fade_timer = 0;
float music_fade_total_time = 60 * 2;

void init_music() {
  main_bgm = LoadMusicStream("assets/stargaze.ogg");
  menu_bgm = LoadMusicStream("assets/crazy_space.ogg");
  PlayMusicStream(menu_bgm);
}

void update_music() {
  UpdateMusicStream(menu_bgm);
  UpdateMusicStream(main_bgm);

  if (music_fade_timer > 0) {
    music_fade_timer--;
    float percent = music_fade_timer / music_fade_total_time;
    SetMusicVolume(menu_bgm, percent);
    SetMusicVolume(main_bgm, 1 - percent);
  }
}

void fade_to_main_bgm_music() {
  PlayMusicStream(main_bgm);
  music_fade_timer = music_fade_total_time;
}