#include "raylib.h"
#include "raymath.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

#include "src/constants.c"
#include "src/stars.c"
#include "src/meteors.c"
#include "src/camera.c"
#include "src/planet.c"
#include "src/bullets.c"

char *game_title = "Super Asteroids Destroyer";

typedef enum {
  main_menu,
  controls_menu,
  playing,
  paused,
  game_over,
} Game_State;

typedef enum {
  play,
  controls,
  exit,
  /// @todo: sub_menu? is there a better way to do this?
  /// there is already a controls_menu
  /// maybe not ideal but create another enum
  /// with just this option, since there is only one option
  /// on the controls menu, or maybe just a boolean
  /// the real problem is that the selection
  /// is just one variable, which makes sense.
  controls_back,
} Menu_State;

/// @todo: is this really necessary?
Menu_State menu_states[3] = {
  play,
  controls,
  exit,
};

Color AQUA = {0, 255, 255, 255};
Color SEA_GREEN = {60, 179, 113, 255};
Color ORANGE_RED = {255, 69, 0, 255};

typedef struct {
  Vector2 position;
  Vector2 velocity;
  int timeout;
} Fire_Particle;

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

  Music main_bgm = LoadMusicStream("assets/stargaze.ogg");
  Music menu_bgm = LoadMusicStream("assets/crazy_space.ogg");
  PlayMusicStream(menu_bgm);
  int music_fade_timer = 0;
  float music_fade_total_time = 60 * 2;

  Sound click_sfx = LoadSound("assets/click.wav");
  Sound select_sfx = LoadSound("assets/select.wav");
  int select_effect_timer = 0;
  float select_effect_total_time = 60 * 0.5;

  Texture2D key_a = LoadTexture("assets/keyboard_a.png");
  Texture2D key_d = LoadTexture("assets/keyboard_d.png");
  Texture2D key_w = LoadTexture("assets/keyboard_w.png");
  Texture2D key_enter = LoadTexture("assets/keyboard_enter.png");
  Texture2D key_space = LoadTexture("assets/keyboard_space.png");

  init_camera();
  init_stars();
  init_meteors();
  init_planet();
  init_bullets();

  Sound hurt_sfx = LoadSound("assets/hurt.wav");
  Texture2D ship_texture = LoadTexture("assets/ship.png");
  Vector2 ship_position = screen_center;
  Vector2 ship_velocity = {0, 0};
  Vector2 ship_acceleration = {300, 300};
  float ship_rotation_deg = 0;
  int rotation_speed = 500;

  #define total_fire_particles 200
  Fire_Particle fire_particles[total_fire_particles] = {};

  int slowmotion_timer = 0;

  Game_State game_state = main_menu;
  Menu_State menu_state = play;
  int menu_state_index = 0;

  int score = 0;
  int energy = 3;

  Sound explosion_sfx = LoadSound("assets/boom.wav");

  /// @todo: remove or use a cool gradient in wave.fs
  Shader wave_shader = LoadShader("assets/wave.vs", 0);

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    /// @todo: uncomment this
    // UpdateMusicStream(menu_bgm);
    // UpdateMusicStream(main_bgm);

    if (music_fade_timer > 0) {
      music_fade_timer--;
      float percent = music_fade_timer / music_fade_total_time;
      SetMusicVolume(menu_bgm, percent);
      SetMusicVolume(main_bgm, 1 - percent);
    }

    if (select_effect_timer > 0) {
      select_effect_timer--;
      if (select_effect_timer == 0) {
        if (menu_state == play)
          game_state = playing;
        else if (menu_state == controls) {
          game_state = controls_menu;
          menu_state = controls_back;
        } else if (menu_state == exit) {
          CloseWindow();
        } else if (menu_state == controls_back) {
          game_state = main_menu;
          menu_state = controls;
        }
      }
    }

    bool is_slowmotion = slowmotion_timer > 0;
    float slowmotion_factor = is_slowmotion ? 0.05 : 1;
    if(is_slowmotion) {
      slowmotion_timer--;
    }

    if(IsKeyPressed(KEY_J)) {
      current_shake_strength = shake_strength;
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
        float radians = ship_rotation_deg * DEG2RAD;
        Vector2 direction = {sin(radians), -cos(radians)};
        ship_velocity.x += direction.x * ship_acceleration.x * dt;
        ship_velocity.y += direction.y * ship_acceleration.y * dt;

        /// @todo: input is not affected by slowdown
        score += 10;
        for(int i = 0; i < total_fire_particles; i++) {
          fire_particles[i].position = (Vector2){GetRandomValue(ship_position.x - 5, ship_position.x + 5), GetRandomValue(ship_position.y - 5, ship_position.y + 5)};
          fire_particles[i].velocity = (Vector2){direction.x * 100 * -5, direction.y * 100 * -5};
          fire_particles[i].position.x += fire_particles[i].velocity.x * dt * slowmotion_factor;
          fire_particles[i].position.y += fire_particles[i].velocity.y * dt * slowmotion_factor;
          fire_particles[i].timeout = 0.1 * 60;
        }
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
        menu_state_index--;
        if (menu_state_index == -1)
          menu_state_index = 2;
        menu_state = menu_states[menu_state_index];
      }
    }
    if (IsKeyPressed(KEY_S) && select_effect_timer == 0) {
      if (game_state == main_menu) {
        PlaySound(click_sfx);
        menu_state_index++;
        if (menu_state_index == 3)
          menu_state_index = 0;
        menu_state = menu_states[menu_state_index];
      }
    }

    if (IsKeyPressed(KEY_ENTER)) {
      if (game_state == main_menu) {
        if (menu_state == play) {
          PlaySound(select_sfx);
          select_effect_timer = select_effect_total_time;
          PlayMusicStream(main_bgm);
          music_fade_timer = music_fade_total_time;
        } else if (menu_state == controls) {
          PlaySound(select_sfx);
          select_effect_timer = select_effect_total_time;
        } else if (menu_state == exit) {
          PlaySound(select_sfx);
          select_effect_timer = select_effect_total_time;
        }
      } else if (game_state == controls_menu) {
        if (menu_state == controls_back) {
          PlaySound(select_sfx);
          select_effect_timer = select_effect_total_time;
        }
      } else if(game_state == game_over) {
        PlaySound(select_sfx);
        game_state = playing;
        ship_position = (Vector2){half_screen_width, half_screen_height};
        ship_velocity = (Vector2){0, 0};
        ship_rotation_deg = 0;
        energy = 3;
        score = 0;
        reset_meteors();
        reset_bullets();
      }
    }

    update_camera(dt);
    update_stars();
    update_planet();

    if(game_state == playing) {
      ship_position.x += ship_velocity.x * dt * slowmotion_factor;
      ship_position.y += ship_velocity.y * dt * slowmotion_factor;
      ship_position.x = Wrap(ship_position.x, 0, screen_width);
      ship_position.y = Wrap(ship_position.y, 0, screen_height);

      for(int i = 0; i < total_fire_particles; i++) {
        if(fire_particles[i].timeout > 0) {
          if(false){
          fire_particles[i].position.x += fire_particles[i].velocity.x * dt * slowmotion_factor;
          fire_particles[i].position.y += fire_particles[i].velocity.y * dt * slowmotion_factor;
          Vector2 norm_pos = Vector2Normalize(fire_particles[i].position);
          fire_particles[i].position = (Vector2){
            Clamp(fire_particles[i].position.x, fire_particles[i].position.x, norm_pos.x * 5),
            Clamp(fire_particles[i].position.y, fire_particles[i].position.y, norm_pos.y * 5),
          };
          }
          fire_particles[i].timeout -= dt;
        } else if(fire_particles[i].timeout == 0) {
          if(false){
          fire_particles[i].position = (Vector2){-100, -100};
          fire_particles[i].velocity = (Vector2){0, 0};
          }
          fire_particles[i].timeout = -1;
        }
      }

      update_bullets(dt, slowmotion_factor);

      for(int i = 0; i < total_meteors; i++) {
        meteors[i].position.x += meteors[i].velocity.x * dt * slowmotion_factor;
        meteors[i].position.y += meteors[i].velocity.y * dt * slowmotion_factor;

        for(int j = 0; j < total_explosion_particles; j++) {
          if(meteors[i].explosion_particles[j].timeout > 0) {
            if(!is_slowmotion)
              meteors[i].explosion_particles[j].timeout--;
            meteors[i].explosion_particles[j].position.x += meteors[i].explosion_particles[j].velocity.x * dt * slowmotion_factor;
            meteors[i].explosion_particles[j].position.y += meteors[i].explosion_particles[j].velocity.y * dt * slowmotion_factor;
          } else if(meteors[i].explosion_particles[j].timeout == 0) {
            meteors[i].explosion_particles[j].position = (Vector2){-1000, -1000};
            meteors[i].explosion_particles[j].velocity = (Vector2){0, 0};
            meteors[i].explosion_particles[j].timeout = -1;
          }
        }

        if(is_out_of_bounds(meteors[i].position)) {
          spawn_meteor(&meteors[i]);
        }

        Vector2 meteor_center = {meteors[i].position.x + meteors[i].texture.width / 2.0, meteors[i].position.y + meteors[i].texture.height / 2.0};
        if(CheckCollisionCircles(meteor_center, meteors[i].radius, ship_position, 10)) {
          for(int j = 0; j < total_explosion_particles; j++) {
            float radians = j * DEG2RAD;
            meteors[i].explosion_particles[j] = (Explosion_Particle){
              meteor_center,
              (Vector2){cos(radians) * GetRandomValue(50, 150), -sin(radians) * GetRandomValue(50, 150)},
              1 * 60,
            };
          }
          if(--energy == 0) game_state = game_over;
          spawn_meteor(&meteors[i]);
          PlaySound(hurt_sfx);
          score += 100;
          slowmotion_timer = 1 * 60;
          current_shake_strength = shake_strength;
        }
        for(int j = 0; j < total_bullets; j++) {
          if(CheckCollisionCircles(meteor_center, meteors[i].radius, bullets[j].position, bullet_radius)) {
            current_shake_strength = shake_strength;
            score += 100;
            PlaySound(explosion_sfx);
            set_explosion_particles(meteor_center, &meteors[i]);
            spawn_meteor(&meteors[i]);
            reset_bullet(j);
          }
        }
      }
    }

    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode2D(camera);

    draw_stars();
    draw_planet();

    if (game_state == playing) {
      draw_meteors();

      for(int i = 0; i < total_fire_particles; i++) {
        if(fire_particles[i].timeout > 0)
          DrawCircleV(fire_particles[i].position, 2, RED);
      }

      draw_bullets();

      DrawTexturePro(ship_texture,
        (Rectangle){0, 0, ship_texture.width, ship_texture.height},
        (Rectangle){ship_position.x, ship_position.y, ship_texture.width, ship_texture.height},
        (Vector2){ship_texture.width / 2.0, ship_texture.height / 2.0},
        ship_rotation_deg, WHITE);

      int x = 20, y = 10;
      int width = 140, height = 40;
      int width_gap = 4, height_gap = 5;
      int thickness = 5;
      Color color = SEA_GREEN;
      switch(energy) {
        case 2: color = ORANGE; break;
        case 1: color = ORANGE_RED; break;
      }
      float energy_block_size = (width - thickness * 2 - width_gap * 4) / 3.0;
      DrawRectangleLinesEx((Rectangle){x, y, width, height}, thickness, color);
      for(int i = 0; i < energy; i++) {
        DrawRectangleRec((Rectangle){
            x + thickness + width_gap * (i + 1) + energy_block_size * i,
            y + thickness + height_gap,
            energy_block_size,
            height - thickness * 2 - height_gap * 2,
          }, color);
      }

      draw_text(font, TextFormat("Score %d", score), (Vector2){280, 15}, WHITE);
    }

    float elapsedTime = GetTime();
    // TraceLog(LOG_WARNING, TextFormat("Loc %d", GetShaderLocation(wave_shader, "time")));
    SetShaderValue(wave_shader, GetShaderLocation(wave_shader, "time"), &elapsedTime, SHADER_UNIFORM_FLOAT);
    if(game_state == main_menu || game_state == controls_menu) {
      /// @todo: wave letters
      Color a = {226, 232, 240, 255};
      // BeginShaderMode(wave_shader);
      // draw_text(font_title, game_title, (Vector2){half_screen_width, 100}, AQUA);
      draw_text(font_title, "SUPER ASTEROIDS DESTROYER", (Vector2){half_screen_width, half_screen_height - 150}, a);
      // draw_text(font_title, "\tSuper\nAsteroids\nDestroyer", (Vector2){half_screen_width, 100}, WHITE);

      // draw_text(font_title, "S", (Vector2){half_screen_width, 100}, WHITE);
      // EndShaderMode();

      // elapsedTime += 1;
      // SetShaderValue(wave_shader, GetShaderLocation(wave_shader, "time"), &elapsedTime, SHADER_UNIFORM_FLOAT);
      // BeginShaderMode(wave_shader);
      // draw_text(font_title, "u", (Vector2){half_screen_width + 20, 100}, WHITE);
      // EndShaderMode();

      // draw_text(font_title, "p", (Vector2){half_screen_width + 40, 100}, WHITE);
      // draw_text(font_title, "e", (Vector2){half_screen_width + 60, 100}, WHITE);
      // draw_text(font_title, "r", (Vector2){half_screen_width + 80, 100}, WHITE);

      // EndShaderMode();
    }

    if (game_state == main_menu) {
      bool effect_timer = 8 < select_effect_timer && 24 < select_effect_timer;

      draw_text(font, "Start", (Vector2){half_screen_width, half_screen_height},
                 menu_state == play && !effect_timer ? AQUA : WHITE);
      draw_text(
          font, "Controls", (Vector2){half_screen_width, half_screen_height + 40},
          menu_state == controls && !effect_timer ? AQUA : WHITE);
      draw_text(font, "Exit", (Vector2){half_screen_width, half_screen_height + 40 * 2},
                menu_state == exit && !effect_timer ? AQUA : WHITE);

    } else if (game_state == controls_menu) {
      bool effect_timer = 8 < select_effect_timer && 24 < select_effect_timer;

      draw_text(font, "Controls", (Vector2){half_screen_width, half_screen_height - 130}, WHITE);

      draw_text(font, "Rotate", (Vector2){half_screen_width - 270, half_screen_height - 50}, WHITE);
      DrawTexture(key_a, half_screen_width - 400, half_screen_height + 60, WHITE);
      DrawTexture(key_d, half_screen_width - 300, half_screen_height + 60, WHITE);

      draw_text(font, "Thrust", (Vector2){half_screen_width - 100, half_screen_height - 50}, WHITE);
      DrawTexture(key_w, half_screen_width - 170, half_screen_height + 60, WHITE);

      draw_text(font, "Shoot", (Vector2){half_screen_width + 100, half_screen_height - 50}, WHITE);
      DrawTexture(key_space, half_screen_width + 30, half_screen_height + 60, WHITE);

      draw_text(font, "Pause", (Vector2){half_screen_width + 250, half_screen_height - 50}, WHITE);
      DrawTexture(key_enter, half_screen_width + 180, half_screen_height + 60, WHITE);

      draw_text(font, "< Back", (Vector2){half_screen_width, half_screen_height + 300},
                menu_state == controls_back && !effect_timer ? AQUA : WHITE);
    } else if(game_state == game_over) {
      draw_text(font, "Game Over", (Vector2){half_screen_width, half_screen_height - 150}, RED);
      draw_text(font, TextFormat("Score: %d", score), (Vector2){half_screen_width, half_screen_height}, AQUA);
      draw_text(font, "Press Enter to play again", (Vector2){half_screen_width, half_screen_height + 30}, WHITE);
    }
    EndMode2D();
    EndDrawing();
  }

  CloseWindow();
  return 0;
}