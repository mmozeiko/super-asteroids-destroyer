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
  // sub menu
  controls_back,
} Menu_State;

Menu_State menu_states[3] = {
    play,
    controls,
    exit,
};

Color AQUA = {0, 255, 255, 255};

typedef struct {
  Vector2 position;
  int size;
} Star;

void draw_text(Font font, char *text, Vector2 position, Color color) {
  int fontsize = 34;
  DrawTextEx(
      font, text,
      (Vector2){position.x - (MeasureText(text, fontsize) / 2.0), position.y},
      fontsize, 1, color);
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

  Texture2D ship_texture = LoadTexture("assets/ship.png");
  Vector2 ship_position = {half_screen_width, half_screen_height};
  Vector2 ship_velocity = {0, 0};
  Vector2 ship_acceleration = {300, 300};
  float ship_rotation_deg = 0;
  int rotation_speed = 500;

  Game_State game_state = main_menu;
  Menu_State menu_state = play;
  int menu_state_index = 0;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    UpdateMusicStream(menu_bgm);
    UpdateMusicStream(main_bgm);

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
      }
    }
    if (IsKeyPressed(KEY_W)) {
      if (game_state == main_menu) {
        PlaySound(click_sfx);
        menu_state_index--;
        if (menu_state_index == -1)
          menu_state_index = 2;
        menu_state = menu_states[menu_state_index];
      }
    }
    if (IsKeyPressed(KEY_S)) {
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

    ship_position.x += ship_velocity.x * dt;
    ship_position.y += ship_velocity.y * dt;
    ship_position.x = Wrap(ship_position.x, 0, screen_width);
    ship_position.y = Wrap(ship_position.y, 0, screen_height);

    for (int i = 0; i < total_stars; i++) {
      stars[i].position.x++;
      stars[i].position.y++;
      stars[i].position.x = Wrap(stars[i].position.x, 0, screen_width);
      stars[i].position.y = Wrap(stars[i].position.y, 0, screen_height);
    }

    BeginDrawing();
    ClearBackground(BLACK);
    for (int i = 0; i < total_stars; i++) {
      DrawCircleV(stars[i].position, stars[i].size, GRAY);
    }

    if (game_state == playing) {
      DrawTexturePro(
          ship_texture,
          (Rectangle){0, 0, ship_texture.width, ship_texture.height},
          (Rectangle){ship_position.x, ship_position.y,
                      ship_texture.width, ship_texture.height},
          (Vector2){ship_texture.width / 2.0, ship_texture.height / 2.0},
          ship_rotation_deg, WHITE);
    }

    if(game_state == main_menu || game_state == controls_menu) {
      /// @todo: for some reason gama title doesn't look like it's centered
      /// that's why I added a 30 pixel left padding
      draw_text(font, game_title, (Vector2){half_screen_width + 30, 100}, WHITE);
    }

    if (game_state == main_menu) {
      int effect_timer = 8 < select_effect_timer && 24 < select_effect_timer;

      draw_text(font, "Start",
                (Vector2){half_screen_width, half_screen_height},
                effect_timer         ? WHITE
                : menu_state == play ? AQUA
                                     : WHITE);
      draw_text(
          font, "Controls",
          (Vector2){half_screen_width ,
                    half_screen_height + 40},
          effect_timer             ? WHITE
          : menu_state == controls ? AQUA
                                   : WHITE);
      draw_text(font, "Exit",
                (Vector2){half_screen_width,
                          half_screen_height + 40 * 2},

                effect_timer         ? WHITE
                : menu_state == exit ? AQUA
                                     : WHITE);
    } else if (game_state == controls_menu) {
      int effect_timer = 8 < select_effect_timer && 24 < select_effect_timer;

      draw_text(font, "Controls", (Vector2){half_screen_width, half_screen_height - 130}, WHITE);

      draw_text(font, "Rotate", (Vector2){half_screen_width - 270, half_screen_height - 50}, WHITE);

      DrawTexture(key_a, half_screen_width - 400, half_screen_height + 60, WHITE);
      DrawTexture(key_d,half_screen_width - 300, half_screen_height + 60, WHITE);

      draw_text(font, "Thrust",
          (Vector2){half_screen_width - 100,
                    half_screen_height - 50}, WHITE);

      DrawTexture(key_w, half_screen_width - 170, half_screen_height + 60, WHITE);

      draw_text(
          font, "Shoot",
          (Vector2){half_screen_width + 100,
                    half_screen_height - 50},
          WHITE);

      DrawTexture(key_space,  half_screen_width + 30, half_screen_height + 60, WHITE);

      draw_text(font, "Pause",
          (Vector2){half_screen_width + 250,
                    half_screen_height - 50},
          WHITE);

      DrawTexture(key_enter,  half_screen_width + 180, half_screen_height + 60, WHITE);

      draw_text(font, "< Back",
                (Vector2){half_screen_width,
                          half_screen_height + 300},
                effect_timer                  ? WHITE
                : menu_state == controls_back ? AQUA
                                              : WHITE);
    }
    EndDrawing();
  }

  CloseWindow();
  return 0;
}