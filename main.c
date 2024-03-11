#include "raylib.h"
#include "raymath.h"

#define STB_PERLIN_IMPLEMENTATION
#include "vendor/stb_perlin.h"

#include "src/constants.c"
#include "src/camera.c"
#include "src/stars.c"
#include "src/meteors.c"
#include "src/planet.c"
#include "src/bullets.c"
#include "src/ship.c"
#include "src/controls_menu.c"
#include "src/music.c"
#include "src/score.c"

typedef enum {
  main_menu,
  controls_menu,
  playing,
  paused,
  hit_stop,
  game_over,
} Game_State;

typedef enum {
  play,
  controls,
  exit,
} Menu_State;

void draw_text(Font font, const char *text, Vector2 position, Color color) {
  int font_size = 34, spacing = 1;
  DrawTextEx(font, text, (Vector2){position.x - (MeasureTextEx(font, text, font_size, spacing).x / 2.0), position.y}, font_size, spacing, color);
}

bool is_out_of_bounds(Vector2 position) {
  float despawn_offset = 500;
  float top_despawn_zone = -despawn_offset;
  float right_despawn_zone = screen_width + despawn_offset;
  float bottom_despawn_zone = screen_height + despawn_offset;
  float left_despawn_zone = -despawn_offset;

  if(position.y < top_despawn_zone
  || position.x > right_despawn_zone
  || position.y > bottom_despawn_zone
  || position.x < left_despawn_zone) {
    return true;
  }

  return false;
}

int main() {
  SetTraceLogLevel(LOG_WARNING);
  InitWindow(screen_width, screen_height, game_title);
  SetTargetFPS(60);
  InitAudioDevice();

  Font font = LoadFontEx("assets/kenney_pixel.ttf", 34, 0, 250);
  SetTextLineSpacing(34);

  Font font_title = LoadFontEx("assets/not_jam_slab_14.ttf", 34, 0, 250);
  SetTextLineSpacing(34);

  Sound hurt_sfx = LoadSound("assets/hurt.wav");
  Sound click_sfx = LoadSound("assets/click.wav");
  Sound select_sfx = LoadSound("assets/select.wav");
  float select_effect_timer = 0;
  float select_effect_total_time = 0.6;

  init_music();
  init_controls_menu();
  init_camera();
  init_stars();
  init_meteors();
  init_planet();
  init_bullets();
  init_ship();

  float slowmotion_timer = 0;

  Game_State game_state = main_menu;
  Menu_State menu_state = play;
  score_t highscore = load_highscore();

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    update_music(dt);

    if (select_effect_timer > 0) {
      select_effect_timer -= dt;
      if (select_effect_timer < 0) {
        select_effect_timer = 0;
        switch(game_state) {
          case playing: break;
          case paused: break;
          case hit_stop: break;
          case game_over: break;
          case controls_menu: {
            game_state = main_menu;
            break;
          }
          case main_menu: {
            switch(menu_state) {
              case play: {
                game_state = playing;
                break;
              }
              case controls: {
                game_state = controls_menu;
                break;
              }
              case exit: {
                CloseWindow();
                break;
              }
            }
            break;
          }
        }
      }
    }

    bool is_slowmotion = slowmotion_timer > 0;
    float slowmotion_factor = is_slowmotion ? 0.05 : 1;
    if(is_slowmotion) {
      slowmotion_timer -= dt;
    }
    if(!is_slowmotion && game_state == hit_stop) {
      game_state = game_over;
      if(score > highscore) {
        save_highscore();
        highscore = score;
      }
    }

    if (IsKeyDown(KEY_A)) {
      if (game_state == playing)
        ship_rotation_deg -= rotation_speed * dt * slowmotion_factor;
    }
    if (IsKeyDown(KEY_D)) {
      if (game_state == playing)
        ship_rotation_deg += rotation_speed * dt * slowmotion_factor;
    }
    if (IsKeyDown(KEY_W)) {
      if (game_state == playing) {
        accelerate_ship(dt, slowmotion_factor);
        score += velocity_score;
      }
    }
    if(IsKeyPressed(KEY_SPACE)) {
      if (game_state == playing) {
        shoot_bullet(ship_position, ship_rotation_deg);
      }
    }
    if (IsKeyPressed(KEY_W) && select_effect_timer == 0) {
      if (game_state == main_menu) {
        PlaySound(click_sfx);
        if(--menu_state == -1) menu_state = 2;
      }
    }
    if (IsKeyPressed(KEY_S) && select_effect_timer == 0) {
      if (game_state == main_menu) {
        PlaySound(click_sfx);
        if(++menu_state == 3) menu_state = 0;
      }
    }

    if (IsKeyPressed(KEY_ENTER)) {
      if (game_state == main_menu) {
        PlaySound(select_sfx);
        select_effect_timer = select_effect_total_time;
        if (menu_state == play) {
          fade_to_main_bgm_music();
        }
      } else if (game_state == controls_menu) {
        PlaySound(select_sfx);
        select_effect_timer = select_effect_total_time;
      } else if(game_state == game_over) {
        PlaySound(select_sfx);
        game_state = playing;
        score = 0;
        reset_meteors();
        reset_bullets();
        reset_ship();
      } else if(game_state == playing) {
        game_state = paused;
      } else if(game_state == paused) {
        game_state = playing;
      }
    }

    update_camera(dt);
    update_stars();
    update_planet(dt);

    if(game_state == playing) {
      update_ship(dt, slowmotion_factor);
      update_bullets(dt, slowmotion_factor);

      for(int i = 0; i < total_meteors; i++) {
        meteors[i].position.x += meteors[i].velocity.x * dt * slowmotion_factor;
        meteors[i].position.y += meteors[i].velocity.y * dt * slowmotion_factor;

        update_explosion_particles(&meteors[i], dt, is_slowmotion, slowmotion_factor);

        if(is_out_of_bounds(meteors[i].position)) {
          spawn_meteor(&meteors[i]);
        }

        Vector2 meteor_center = {meteors[i].position.x + meteors[i].texture.width / 2.0, meteors[i].position.y + meteors[i].texture.height / 2.0};
        if(CheckCollisionCircles(meteor_center, meteors[i].radius, ship_position, ship_radius)) {
          PlaySound(hurt_sfx);
          set_explosion_particles(meteor_center, &meteors[i]);
          spawn_meteor(&meteors[i]);
          if(--energy == 0) {
            game_state = hit_stop;
            StopMusicStream(main_bgm);
            PlaySound(lose_sfx);
          }
          slowmotion_timer = 1;
          update_score_meteor();
          shake_camera();
        }
        bullets_check_collision_with_meteor(meteor_center, &meteors[i]);
      }
    }

    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode2D(camera);

    draw_stars();
    draw_planet(dt);

    if (game_state == playing || game_state == hit_stop) {
      draw_meteors();
      draw_bullets();
      draw_ship();
      draw_energy();
      draw_text(font, TextFormat("Score %d", score), (Vector2){280, 15}, WHITE);
    }
    EndMode2D();

    bool effect_timer = 0.1 < select_effect_timer && 0.5 < select_effect_timer;
    if (game_state == main_menu) {
      draw_text(font_title, TextToUpper(game_title), (Vector2){half_screen_width, half_screen_height - 150}, (Color){226, 232, 240, 255});

      draw_text(font, "Start", (Vector2){half_screen_width, half_screen_height},
                 menu_state == play && !effect_timer ? AQUA : WHITE);
      draw_text(
          font, "Controls", (Vector2){half_screen_width, half_screen_height + 40},
          menu_state == controls && !effect_timer ? AQUA : WHITE);
      draw_text(font, "Exit", (Vector2){half_screen_width, half_screen_height + 40 * 2},
                menu_state == exit && !effect_timer ? AQUA : WHITE);
    } else if (game_state == controls_menu) {
      draw_controls_menu(font, effect_timer);
    } else if(game_state == game_over) {
      draw_text(font_title, "GAME OVER", (Vector2){half_screen_width, half_screen_height - 150}, RED);
      draw_text(font, TextFormat("Score: %d", score), (Vector2){half_screen_width, half_screen_height - 50}, AQUA);
      draw_text(font, TextFormat("Highscore: %d", highscore), (Vector2){half_screen_width, half_screen_height - 100}, AQUA);
      draw_text(font, "Press Enter to play again", (Vector2){half_screen_width, half_screen_height}, WHITE);
    } else if(game_state == paused) {
      draw_text(font_title, "PAUSED", (Vector2){half_screen_width, half_screen_height}, WHITE);
    }
    EndDrawing();
  }

  CloseWindow();
  return 0;
}