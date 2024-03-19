Texture2D ship_texture;
Vector2 ship_position;
Vector2 ship_velocity = {0, 0};
Vector2 ship_acceleration = {300, 300};
float ship_rotation_deg = 0;
int rotation_speed = 500;
int ship_radius = 10;
Texture2D fire_texture;

int energy = 3;
Sound lose_sfx;

bool is_fire_visible = false;

void init_ship() {
  lose_sfx = LoadSound("assets/lose.wav");
  fire_texture = LoadTexture("assets/fire.png");
  ship_texture = LoadTexture("assets/ship.png");
  SetTextureFilter(fire_texture, TEXTURE_FILTER_BILINEAR);
  SetTextureFilter(ship_texture, TEXTURE_FILTER_BILINEAR);
  ship_position = screen_center;
}

void accelerate_ship(float dt, float slowmotion_factor) {
  float radians = ship_rotation_deg * DEG2RAD;
  Vector2 direction = {sin(radians), -cos(radians)};
  ship_velocity.x += direction.x * ship_acceleration.x * dt * slowmotion_factor;
  ship_velocity.y += direction.y * ship_acceleration.y * dt * slowmotion_factor;
  is_fire_visible = true;
}

void update_ship(float dt, float slowmotion_factor) {
  ship_position.x += ship_velocity.x * dt * slowmotion_factor;
  ship_position.y += ship_velocity.y * dt * slowmotion_factor;
  ship_position.x = Wrap(ship_position.x, 0, screen_width);
  ship_position.y = Wrap(ship_position.y, 0, screen_height);
}

void reset_ship() {
  ship_position = screen_center;
  ship_velocity = (Vector2){0, 0};
  ship_rotation_deg = 0;
  energy = 3;
}

void draw_ship() {
  if(is_fire_visible) {
    DrawTexturePro(fire_texture,
      (Rectangle){0, 0, fire_texture.width, fire_texture.height},
      (Rectangle){ship_position.x, ship_position.y, fire_texture.width, fire_texture.height},
      (Vector2){fire_texture.width / 2.0, 0},
      ship_rotation_deg, WHITE);
      is_fire_visible = false;
  }
  DrawTexturePro(ship_texture,
    (Rectangle){0, 0, ship_texture.width, ship_texture.height},
    (Rectangle){ship_position.x, ship_position.y, ship_texture.width, ship_texture.height},
    (Vector2){ship_texture.width / 2.0, ship_texture.height / 2.0},
    ship_rotation_deg, WHITE);
}

void draw_energy() {
  Color SEA_GREEN = {60, 179, 113, 255};
  Color ORANGE_RED = {255, 69, 0, 255};
  int x = 20, y = 10;
  int width = 140, height = 40;
  int width_gap = 4, height_gap = 5;
  int thickness = 5;
  Color color = SEA_GREEN;
  switch(energy) {
    case 2: color = ORANGE; break;
    case 1:
    case 0: color = ORANGE_RED; break;
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
}