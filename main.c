#include "raylib.h"
#include "raymath.h"

const char *game_title = "Super Asteroids Destroyer";
const int screen_width = 1280;
const int screen_height = 720;
int half_screen_width = screen_width / 2;
int half_screen_height = screen_height / 2;

typedef enum {
  menu,
  playing,
  paused,
  game_over,
} Game_State;

typedef enum {
  play,
  controls,
  exit,
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

int main() {
  SetTraceLogLevel(LOG_WARNING);
  InitWindow(screen_width, screen_height, game_title);
  SetTargetFPS(60);

  Font font = LoadFontEx("assets/kenney_pixel.ttf", 34, 0, 250);
  SetTextLineSpacing(34);

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
  Vector2 ship_position = {half_screen_width, half_screen_width};
  Vector2 ship_velocity = {0, 0};
  Vector2 ship_acceleration = {300, 300};
  float ship_rotation_deg = 0;
  int rotation_speed = 500;

  Game_State game_state = menu;
  Menu_State menu_state = play;
  int menu_state_index = 0;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

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
      if (game_state == menu) {
        menu_state_index--;
        if (menu_state_index == -1)
          menu_state_index = 2;
        menu_state = menu_states[menu_state_index];
      }
    }
    if (IsKeyPressed(KEY_S)) {
      if (game_state == menu) {
        menu_state_index++;
        if (menu_state_index == 3)
          menu_state_index = 0;
        menu_state = menu_states[menu_state_index];
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
          (Rectangle){ship_position.x, ship_position.y, ship_texture.width,
                      ship_texture.height},
          (Vector2){ship_texture.width / 2.0, ship_texture.height / 2.0},
          ship_rotation_deg, WHITE);
    }

    if (game_state == menu) {

      DrawTextEx(font, "Start",
                 (Vector2){half_screen_width - (MeasureText("Start", 34) / 2.0),
                           half_screen_height},
                 34, 1, menu_state == play ? AQUA : WHITE);
      DrawTextEx(
          font, "Controls",
          (Vector2){half_screen_width - (MeasureText("Controls", 34) / 2.0),
                    half_screen_height + 40},
          34, 1, menu_state == controls ? AQUA : WHITE);
      DrawTextEx(font, "Exit",
                 (Vector2){half_screen_width - (MeasureText("Exit", 34) / 2.0),
                           half_screen_height + 40 * 2},
                 34, 1, menu_state == exit ? AQUA : WHITE);

      DrawTextEx(
          font, game_title,
          (Vector2){half_screen_width - (MeasureText(game_title, 34) / 2.0),
                    half_screen_height / 2.0},
          34, 1, WHITE);
    }
    EndDrawing();
  }

  CloseWindow();
  return 0;
}