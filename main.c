#include "raylib.h"
#include "raymath.h"

char *game_title = "Super Asteroids Destroyer";
const int screen_width = 1280;
const int screen_height = 720;
int half_screen_width = screen_width / 2;
int half_screen_height = screen_height / 2;

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
  controls_back,
} Menu_State;

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
  int size;
} Star;

typedef struct {
  Vector2 position;
  Vector2 velocity;
  Texture2D texture;
  int radius;
} Meteor;

void draw_text(Font font, const char *text, Vector2 position, Color color) {
  int font_size = 34, spacing = 1;
  DrawTextEx(font, text, (Vector2){position.x - (MeasureTextEx(font, text, font_size, spacing).x / 2.0), position.y}, font_size, spacing, color);
}

void spawn_meteor(Vector2 meteor_spawn_locations[12], Texture2D meteors_textures[4], Meteor *meteor) {
  meteor->radius = 45;
  int location_index = GetRandomValue(0, 11);
  meteor->position = meteor_spawn_locations[location_index];

  float angle = 0;
  switch(location_index) {
    /// @note: this logic follows the spawn locations order!
    /// top
    case 0x0: angle = GetRandomValue(270, 270 + 60); break;
    case 0x1: angle = GetRandomValue(190, 350); break;
    case 0x2: angle = GetRandomValue(270 - 60, 270); break;
    /// right:
    case 0x3: angle = GetRandomValue(180, 180 + 60); break;
    case 0x4: angle = GetRandomValue(100, 170); break;
    case 0x5: angle = GetRandomValue(180 - 60, 180); break;
    /// bottom:
    case 0x6: angle = GetRandomValue(90 - 60, 90); break;
    case 0x7: angle = GetRandomValue(10, 170); break;
    case 0x8: angle = GetRandomValue(90, 90  + 60); break;
    /// left:
    case 0x9: angle = GetRandomValue(360 - 60, 360); break;
    case 0xa: angle = GetRandomValue(280, 270 + 160); break;
    case 0xb: angle = GetRandomValue(0, 60); break;
  }
  angle *= DEG2RAD;
  int speed = GetRandomValue(50, 250);
  meteor->velocity = (Vector2){
    cos(angle) * speed,
    -sin(angle) * speed,
  };

  meteor->texture = meteors_textures[GetRandomValue(0, 3)];
}

int main() {
  SetTraceLogLevel(LOG_WARNING);
  InitWindow(screen_width, screen_height, game_title);
  SetTargetFPS(60);
  InitAudioDevice();

  Font font = LoadFontEx("assets/kenney_pixel.ttf", 34, 0, 250);
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

  #define total_stars 100
  Star stars[total_stars] = {};
  for (int i = 0; i < total_stars; i++) {
    stars[i].position = (Vector2){
        GetRandomValue(0, screen_width),
        GetRandomValue(0, screen_height),
    };
    stars[i].size = GetRandomValue(1, 3);
  }

  Texture2D meteors_textures[4] = {
    LoadTexture("assets/meteor0.png"),
    LoadTexture("assets/meteor1.png"),
    LoadTexture("assets/meteor2.png"),
    LoadTexture("assets/meteor3.png"),
  };
  /// @note: padding to keep the location 'inside' the screen boundaries
  int spawn_padding = 30;
  /// @note: offset to keep the location outside the screen
  int spawn_offset = 30;
  Vector2 meteor_spawn_locations[12] = {
    /// @note: top
    {spawn_padding, -spawn_offset},
    {half_screen_width, -spawn_offset},
    {screen_width - spawn_padding, -spawn_offset},
    /// @note: right
    {screen_width + spawn_offset, spawn_padding},
    {screen_width + spawn_offset, half_screen_height},
    {screen_width + spawn_offset, screen_height - spawn_padding},
    /// @note: bottom
    {spawn_padding, screen_height + spawn_offset},
    {half_screen_width, screen_height + spawn_offset},
    {screen_width - spawn_padding, screen_height + spawn_offset},
    /// @note: left
    {-spawn_offset, spawn_padding},
    {-spawn_offset, half_screen_height},
    {-spawn_offset, screen_height - spawn_padding},
  };
  #define total_meteors 20
  Meteor meteors[total_meteors] = {};
  for(int i = 0; i < total_meteors; i++) {
    spawn_meteor(meteor_spawn_locations, meteors_textures, &meteors[i]);
  }

  Texture2D ship_texture = LoadTexture("assets/ship.png");
  Vector2 ship_position = {half_screen_width, half_screen_height};
  Vector2 ship_velocity = {0, 0};
  Vector2 ship_acceleration = {300, 300};
  float ship_rotation_deg = 0;
  int rotation_speed = 500;

  Game_State game_state = main_menu;
  Menu_State menu_state = play;
  int menu_state_index = 0;

  int score = 0;
  int energy = 3;

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

    if (IsKeyDown(KEY_A)) {
      if (game_state == playing)
        ship_rotation_deg -= rotation_speed * dt;
    }
    if (IsKeyDown(KEY_D)) {
      if (game_state == playing)
        ship_rotation_deg += rotation_speed * dt;
    }
    if (IsKeyDown(KEY_W)) {
      if (game_state == playing) {
        float radians = ship_rotation_deg * DEG2RAD;
        Vector2 direction = {sin(radians), -cos(radians)};
        ship_velocity.x += direction.x * ship_acceleration.x * dt;
        ship_velocity.y += direction.y * ship_acceleration.y * dt;

        score += 10;
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
      }
    }

    if(game_state == playing) {
      ship_position.x += ship_velocity.x * dt;
      ship_position.y += ship_velocity.y * dt;
      ship_position.x = Wrap(ship_position.x, 0, screen_width);
      ship_position.y = Wrap(ship_position.y, 0, screen_height);

      for(int i = 0; i < total_meteors; i++) {
        meteors[i].position.x += meteors[i].velocity.x * dt;
        meteors[i].position.y += meteors[i].velocity.y * dt;

        int despawn_horizontal_zone_width = 200 + screen_width + 200;
        int despawn_vertical_zone_height = 200 + screen_height + 200;
        int despawn_zone_size = 100;
        Rectangle top_despawn_zone = {-200, -500, despawn_horizontal_zone_width, despawn_zone_size};
        Rectangle right_despawn_zone = {screen_width + 500, -200, despawn_zone_size, despawn_vertical_zone_height};
        Rectangle bottom_despawn_zone = {-200, screen_height + 500, despawn_horizontal_zone_width, despawn_zone_size};
        Rectangle left_despawn_zone = {-500, -200, despawn_zone_size, despawn_vertical_zone_height};
        
        if(CheckCollisionCircleRec(meteors[i].position, meteors[i].radius, top_despawn_zone)
        || CheckCollisionCircleRec(meteors[i].position, meteors[i].radius, right_despawn_zone)
        || CheckCollisionCircleRec(meteors[i].position, meteors[i].radius, bottom_despawn_zone)
        || CheckCollisionCircleRec(meteors[i].position, meteors[i].radius, left_despawn_zone)) {
          TraceLog(LOG_WARNING, "Respawning meteor with index %d", i);
          spawn_meteor(meteor_spawn_locations, meteors_textures, &meteors[i]);
        }
      }
    }

    for (int i = 0; i < total_stars; i++) {
      stars[i].position.x = Wrap(++stars[i].position.x, 0, screen_width);
      stars[i].position.y = Wrap(++stars[i].position.y, 0, screen_height);
    }

    BeginDrawing();
    ClearBackground(BLACK);
    for (int i = 0; i < total_stars; i++) {
      DrawCircleV(stars[i].position, stars[i].size, GRAY);
    }

    if (game_state == playing) {
      for(int i = 0; i < total_meteors; i++) {
        Meteor meteor = meteors[i];
        DrawTextureV(meteor.texture, meteor.position, WHITE);
        DrawCircleLinesV((Vector2){meteor.position.x + meteor.texture.width / 2.0, meteor.position.y + meteor.texture.height / 2.0}, meteor.radius, BLUE);
      }

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

    if(game_state == main_menu || game_state == controls_menu) {
      draw_text(font, game_title, (Vector2){half_screen_width, 100}, WHITE);
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
    }
    EndDrawing();
  }

  CloseWindow();
  return 0;
}