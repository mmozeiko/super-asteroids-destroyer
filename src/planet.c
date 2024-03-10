Texture2D planet_texture;
Texture2D planet_gas_texture;
int gas_rotation = 0;

void init_planet() {
  planet_texture = LoadTexture("assets/planet.png");
  planet_gas_texture = LoadTexture("assets/planet_gas.png");
}

void update_planet() {
  /// @todo: maybe pass dt as param?
  /// @todo: better code? a little confusing
  gas_rotation++;
  if(gas_rotation * 0.01 > 360)
    gas_rotation = 0;
}

void draw_planet() {
  /// @todo: maybe pass dt as param?
  DrawTextureV(planet_texture, (Vector2){half_screen_width - 200, half_screen_height - 100}, WHITE);
  DrawTexturePro(planet_gas_texture,
    (Rectangle){0, 0, planet_gas_texture.width, planet_gas_texture.height},
    (Rectangle){half_screen_width + 450, half_screen_height + 500, planet_gas_texture.width, planet_gas_texture.height},
    (Vector2){planet_gas_texture.width / 2.0, planet_gas_texture.height / 2.0},
    gas_rotation * 0.01, WHITE);
}