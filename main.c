#include "raylib.h"
#include "raymath.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

char *game_title = "Super Asteroids Destroyer";
const int screen_width = 1280;
const int screen_height = 720;
const int half_screen_width = screen_width / 2;
const int half_screen_height = screen_height / 2;
Vector2 screen_center = {half_screen_width, half_screen_height};

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
  int size;
} Star;

typedef struct {
  Vector2 position;
  Vector2 velocity;
  int timeout;
} Fire_Particle;

typedef struct {
  Vector2 position;
  Vector2 velocity;
  int timeout;
} Explosion_Particle;

#define total_explosion_particles 360
typedef struct {
  Vector2 position;
  Vector2 velocity;
  Texture2D texture;
  int radius;
  Explosion_Particle explosion_particles[total_explosion_particles];
} Meteor;

typedef struct {
  Vector2 position;
  Vector2 velocity;
  int radius;
  float rotation;
} Bullet;

void draw_text(Font font, const char *text, Vector2 position, Color color) {
  int font_size = 34, spacing = 1;
  DrawTextEx(font, text, (Vector2){position.x - (MeasureTextEx(font, text, font_size, spacing).x / 2.0), position.y}, font_size, spacing, color);
}

void spawn_meteor(Texture2D meteors_textures[4], Meteor *meteor) {
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

float noise_2d(float x, float y) {
  float z = 0, lacunarity = 2, gain = 0.5, octaves = 3;
  return stb_perlin_fbm_noise3(x, y, z, lacunarity, gain, octaves);
}

static float noise_i = 0;
Vector2 noise_offset(float dt, float speed, float strength) {
  noise_i += dt * speed;
  return (Vector2){
    noise_2d(1, noise_i) * strength,
    noise_2d(100, noise_i) * strength,
  };
}

/// @todo: decide if I will use this
// typedef struct {
//   float noise_i;
//   float strength;
//   float decay_rate;
//   float current_strength;
//   float speed;
//   Vector2 offset;
// } Camera_Shake;

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

  #define total_stars 100
  Star stars[total_stars] = {};
  for (int i = 0; i < total_stars; i++) {
    stars[i].position = (Vector2){
      GetRandomValue(-1000, screen_width + 1000),
      GetRandomValue(-1000, screen_height + 1000),
    };
    stars[i].size = GetRandomValue(1, 3);
  }

  Texture2D meteors_textures[4] = {
    LoadTexture("assets/meteor0.png"),
    LoadTexture("assets/meteor1.png"),
    LoadTexture("assets/meteor2.png"),
    LoadTexture("assets/meteor3.png"),
  };
  #define total_meteors 20
  Meteor meteors[total_meteors] = {};
  for(int i = 0; i < total_meteors; i++) {
    spawn_meteor(meteors_textures, &meteors[i]);
  }

  int despawn_horizontal_zone_width = 200 + screen_width + 200;
  int despawn_vertical_zone_height = 200 + screen_height + 200;
  int despawn_zone_size = 100;
  Rectangle top_despawn_zone = {-200, -500, despawn_horizontal_zone_width, despawn_zone_size};
  Rectangle right_despawn_zone = {screen_width + 500, -200, despawn_zone_size, despawn_vertical_zone_height};
  Rectangle bottom_despawn_zone = {-200, screen_height + 500, despawn_horizontal_zone_width, despawn_zone_size};
  Rectangle left_despawn_zone = {-500, -200, despawn_zone_size, despawn_vertical_zone_height};

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

  Texture2D planet_texture = LoadTexture("assets/planet.png");
  Texture2D planet_gas_texture = LoadTexture("assets/planet_gas.png");
  int gas_rotation = 0;

  Sound explosion_sfx = LoadSound("assets/boom.wav");
  Texture2D bullet_texture = LoadTexture("assets/bullet.png");
  #define total_bullets 100
  Bullet bullets[total_bullets] = {};
  int bullet_index = 0;

  /// @todo: remove
  Shader wave_shader = LoadShader("assets/wave.vs", 0);

  Camera2D camera = {};
  camera.target = screen_center;
  camera.zoom = 1;

  /// @todo: decide if I will use this
  // Camera_Shake camera_shake = {};
  // camera_shake.noise_i = 0;
  // camera_shake.strength = 60 * 0.4;
  // camera_shake.decay_rate = 3;
  // camera_shake.current_strength = 0;
  // camera_shake.speed = 30;
  // camera_shake.offset = (Vector2){0,0};

  float shake_strength = 60 * 0.4;
  float shake_decay_rate = 3;
  float shake_speed = 30;
  float current_shake_strength = 0;
  Vector2 shake_offset = {};

  /// @todo: decide if I will use this
  // float camera_angle = 0;
  // float max_angle = 10;

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
        float radians = ship_rotation_deg * DEG2RAD;
        Vector2 direction = {sin(radians), -cos(radians)};
        bullets[bullet_index] = (Bullet){
          (Vector2){ship_position.x, ship_position.y},
          (Vector2){direction.x * 100 * 5, direction.y * 100 * 5},
          10,
          ship_rotation_deg,
        };
        bullet_index = bullet_index >= total_bullets ? 0 : bullet_index + 1;
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

        for(int i = 0; i < total_meteors; i++) {
          spawn_meteor(meteors_textures, &meteors[i]);
          for(int j = 0; j < total_explosion_particles; j++) {
            meteors[i].explosion_particles[j].position = (Vector2){-1000, -1000};
            meteors[i].explosion_particles[j].velocity = (Vector2){0, 0};
            meteors[i].explosion_particles[j].timeout = -1;
          }
        }

        for(int i = 0; i < total_bullets; i++) {
          bullets[i].position = (Vector2){-1000, -1000};
          bullets[i].velocity = (Vector2){0, 0};
          bullets[i].rotation = 0;
        }
      }
    }

    current_shake_strength = Lerp(current_shake_strength, 0, shake_decay_rate * dt);
    shake_offset = noise_offset(dt, shake_speed, current_shake_strength);
    camera.offset.x = screen_center.x + shake_offset.x;
    camera.offset.y = screen_center.y + shake_offset.y;

    if(game_state == playing) {
      ship_position.x += ship_velocity.x * dt * slowmotion_factor;
      ship_position.y += ship_velocity.y * dt * slowmotion_factor;
      ship_position.x = Wrap(ship_position.x, 0, screen_width);
      ship_position.y = Wrap(ship_position.y, 0, screen_height);

      for(int i = 0; i < total_fire_particles; i++) {
        if(fire_particles[i].timeout > 0) {
      //     fire_particles[i].position.x += fire_particles[i].velocity.x * dt * slowmotion_factor;
      //     fire_particles[i].position.y += fire_particles[i].velocity.y * dt * slowmotion_factor;
      //     // Vector2 norm_pos = Vector2Normalize(fire_particles[i].position);
      //     // fire_particles[i].position = (Vector2){
      //     //   Clamp(fire_particles[i].position.x, fire_particles[i].position.x, norm_pos.x * 5),
      //     //   Clamp(fire_particles[i].position.y, fire_particles[i].position.y, norm_pos.y * 5),
      //     // };
          fire_particles[i].timeout -= dt;
        } else if(fire_particles[i].timeout == 0) {
      //     fire_particles[i].position = (Vector2){-100, -100};
      //     fire_particles[i].velocity = (Vector2){0, 0};
          fire_particles[i].timeout = -1;
        }
      }

      for(int i = 0; i < total_bullets; i++) {
        bullets[i].position.x += bullets[i].velocity.x * dt * slowmotion_factor;
        bullets[i].position.y += bullets[i].velocity.y * dt * slowmotion_factor;

        if(CheckCollisionCircleRec(bullets[i].position, bullets[i].radius, top_despawn_zone)
        || CheckCollisionCircleRec(bullets[i].position, bullets[i].radius, right_despawn_zone)
        || CheckCollisionCircleRec(bullets[i].position, bullets[i].radius, bottom_despawn_zone)
        || CheckCollisionCircleRec(bullets[i].position, bullets[i].radius, left_despawn_zone)) {
          bullets[i] = (Bullet){
            {-1000, -1000},
            {0, 0},
            10,
            false,
          };
        }
      }

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
            // TraceLog(LOG_WARNING, "Resetting particle with id %d", j);
            meteors[i].explosion_particles[j].position = (Vector2){-1000, -1000};
            meteors[i].explosion_particles[j].velocity = (Vector2){0, 0};
            meteors[i].explosion_particles[j].timeout = -1;
          }
        }

        if(CheckCollisionCircleRec(meteors[i].position, meteors[i].radius, top_despawn_zone)
        || CheckCollisionCircleRec(meteors[i].position, meteors[i].radius, right_despawn_zone)
        || CheckCollisionCircleRec(meteors[i].position, meteors[i].radius, bottom_despawn_zone)
        || CheckCollisionCircleRec(meteors[i].position, meteors[i].radius, left_despawn_zone)) {
          // TraceLog(LOG_WARNING, "Respawning meteor with index %d", i);
          spawn_meteor(meteors_textures, &meteors[i]);
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
          spawn_meteor(meteors_textures, &meteors[i]);
          PlaySound(hurt_sfx);
          score += 100;
          slowmotion_timer = 1 * 60;
          current_shake_strength = shake_strength;
        }
        for(int j = 0; j < total_bullets; j++) {
          if(CheckCollisionCircles(meteor_center, meteors[i].radius, bullets[j].position, bullets[j].radius)) {
            current_shake_strength = shake_strength;
            score += 100;
            spawn_meteor(meteors_textures, &meteors[i]);
            PlaySound(explosion_sfx);
            bullets[j] = (Bullet){
              {-1000, -1000},
              {0, 0},
              10,
              0,
            };
            for(int k = 0; k < total_explosion_particles; k++) {
              float radians = k * DEG2RAD;
              meteors[i].explosion_particles[k] = (Explosion_Particle){
                meteor_center,
                (Vector2){cos(radians) * GetRandomValue(50, 150), -sin(radians) * GetRandomValue(50, 150)},
                1 * 60,
              };
            }
          }
        }
      }
    }

    for (int i = 0; i < total_stars; i++) {
      stars[i].position.x = Wrap(++stars[i].position.x, 0, screen_width);
      stars[i].position.y = Wrap(++stars[i].position.y, 0, screen_height);
    }

    BeginDrawing();
    ClearBackground(BLACK);
    // DrawText(TextFormat("(%.2f, %.2f)", camera.offset.x, camera.offset.y), 0, 0, 34, WHITE);
    BeginMode2D(camera);

    for (int i = 0; i < total_stars; i++) {
      DrawCircleV(stars[i].position, stars[i].size, GRAY);
    }

    DrawTextureV(planet_texture, (Vector2){half_screen_width - 200, half_screen_height - 100}, WHITE);
    // DrawTextureEx(planet_gas_texture, (Vector2){half_screen_width - 100, half_screen_height}, gas_rotation, 1, WHITE);
    DrawTexturePro(planet_gas_texture, 
      (Rectangle){0, 0, planet_gas_texture.width, planet_gas_texture.height}, 
      (Rectangle){half_screen_width + 450, half_screen_height + 500, planet_gas_texture.width, planet_gas_texture.height}, 
      (Vector2){planet_gas_texture.width / 2.0, planet_gas_texture.height / 2.0}, 
      gas_rotation * .01, WHITE);
    gas_rotation++;
    if(gas_rotation * .01 > 360) gas_rotation = 0;

    if (game_state == playing) {
      for(int i = 0; i < total_meteors; i++) {
        DrawTextureV(meteors[i].texture, meteors[i].position, WHITE);
        // DrawCircleLinesV((Vector2){meteor.position.x + meteor.texture.width / 2.0, meteor.position.y + meteor.texture.height / 2.0}, meteor.radius, BLUE);
        // DrawText(TextFormat("X %.2f Y %.2f", meteor.position.x, meteor.position.y), meteor.position.x, meteor.position.y, 24, WHITE);
        for(int j = 0; j < total_explosion_particles; j++) {
          if(meteors[i].explosion_particles[j].timeout > 0) {
            DrawCircleV(meteors[i].explosion_particles[j].position, 1, WHITE);
          }
        }
      }

      for(int i = 0; i < total_fire_particles; i++) {
        if(fire_particles[i].timeout > 0)
          DrawCircleV(fire_particles[i].position, 2, RED);
      }

      for(int i = 0; i < total_bullets; i++) {
        Bullet bullet = bullets[i];
        Vector2 bullet_position = bullet.position;
        DrawTexturePro(bullet_texture, 
          (Rectangle){0, 0, bullet_texture.width, bullet_texture.height},
          (Rectangle){bullet_position.x, bullet_position.y, bullet_texture.width, bullet_texture.height},
          (Vector2){bullet_texture.width / 2.0, bullet_texture.height / 2.0},
          bullets[i].rotation, WHITE);
      }

      DrawTexturePro(ship_texture,
        (Rectangle){0, 0, ship_texture.width, ship_texture.height},
        (Rectangle){ship_position.x, ship_position.y, ship_texture.width, ship_texture.height},
        (Vector2){ship_texture.width / 2.0, ship_texture.height / 2.0},
        ship_rotation_deg, WHITE);
      // DrawCircleV(ship_position, 3, MAGENTA);
      // DrawText(TextFormat("X %.2f Y %.2f", ship_position.x, ship_position.y), ship_position.x, ship_position.y, 24, WHITE);

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