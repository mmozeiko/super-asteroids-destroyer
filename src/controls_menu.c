Texture2D key_a;
Texture2D key_d;
Texture2D key_w;
Texture2D key_enter;
Texture2D key_space;

/// @todo: remove
// typedef enum {
//   back,
// } Controls_Menu_State;
// Controls_Menu_State controls_menu_state = back;

void init_controls_menu() {
  key_a = LoadTexture("assets/keyboard_a.png");
  key_d = LoadTexture("assets/keyboard_d.png");
  key_w = LoadTexture("assets/keyboard_w.png");
  key_enter = LoadTexture("assets/keyboard_enter.png");
  key_space = LoadTexture("assets/keyboard_space.png");
}

/// @todo: hack, but works!
void draw_text(Font font, const char *text, Vector2 position, Color color);

void draw_controls_menu(Font font, bool effect_timer) {
  /// @todo: use screen center?
  draw_text(font, "Controls", (Vector2){half_screen_width, 70}, WHITE);
  int text_y = 230;
  int keys_y = text_y + 50;

  draw_text(font, "Rotate", (Vector2){half_screen_width - 270, text_y}, WHITE);
  DrawTexture(key_a, half_screen_width - 400, keys_y, WHITE);
  DrawTexture(key_d, half_screen_width - 300, keys_y, WHITE);

  draw_text(font, "Thrust", (Vector2){half_screen_width - 100, text_y}, WHITE);
  DrawTexture(key_w, half_screen_width - 170, keys_y, WHITE);

  draw_text(font, "Shoot", (Vector2){half_screen_width + 100, text_y}, WHITE);
  DrawTexture(key_space, half_screen_width + 30, keys_y, WHITE);

  draw_text(font, "Pause", (Vector2){half_screen_width + 250, text_y}, WHITE);
  DrawTexture(key_enter, half_screen_width + 180, keys_y, WHITE);

  draw_text(font, "< Back", (Vector2){half_screen_width, 140 + 300}, !effect_timer ? AQUA : WHITE);
}