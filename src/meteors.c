typedef struct {
  Vector2 position;
  Vector2 velocity;
} Explosion_Particle;

#define total_explosion_particles 360
typedef struct {
  Vector2 position;
  Vector2 velocity;
  Texture2D texture;
  int radius;
  float particles_timer;
  Explosion_Particle explosion_particles[total_explosion_particles];
} Meteor;

void spawn_meteor(Meteor *meteor);

#define total_meteors_textures 4
Texture2D meteors_textures[total_meteors_textures] = {};

#define total_meteors 20
Meteor meteors[total_meteors] = {};

void init_meteors() {
  for(int i = 0; i < total_meteors_textures; i++) {
    meteors_textures[i] = LoadTexture(TextFormat("assets/meteor%d.png", i));
    SetTextureFilter(meteors_textures[i], TEXTURE_FILTER_BILINEAR);
  }

  for(int i = 0; i < total_meteors; i++) {
    spawn_meteor(&meteors[i]);
  }
}

float explosion_particle_total_timeout = 1;
void set_explosion_particles(Vector2 meteor_center, Meteor* meteor) {
  meteor->particles_timer = explosion_particle_total_timeout;
  for(int i = 0; i < total_explosion_particles; i++) {
    float radians = i * DEG2RAD;
    meteor->explosion_particles[i] = (Explosion_Particle){
      meteor_center,
      (Vector2){cos(radians) * GetRandomValue(50, 150), -sin(radians) * GetRandomValue(50, 150)},
    };
  }
}

void update_explosion_particles(Meteor* meteor, float dt, bool is_slowmotion, float slowmotion_factor) {
  if(meteor->particles_timer > 0) {
    meteor->particles_timer -= dt * slowmotion_factor;
  } else {
    meteor->particles_timer = -1;
  }
  for(int i = 0; i < total_explosion_particles; i++) {
    if(meteor->particles_timer > 0) {
      meteor->explosion_particles[i].position.x += meteor->explosion_particles[i].velocity.x * dt * slowmotion_factor;
      meteor->explosion_particles[i].position.y += meteor->explosion_particles[i].velocity.y * dt * slowmotion_factor;
    } else {
      meteor->explosion_particles[i].position = (Vector2){-1000, -1000};
      meteor->explosion_particles[i].velocity = (Vector2){0, 0};
    }
  }
}

void reset_meteors() {
  for(int i = 0; i < total_meteors; i++) {
    spawn_meteor(&meteors[i]);
    meteors[i].particles_timer = -1;
    for(int j = 0; j < total_explosion_particles; j++) {
      meteors[i].explosion_particles[j].position = (Vector2){-1000, -1000};
      meteors[i].explosion_particles[j].velocity = (Vector2){0, 0};
    }
  }
}

void draw_meteors() {
  for(int i = 0; i < total_meteors; i++) {
    DrawTextureV(meteors[i].texture, meteors[i].position, WHITE);
    for(int j = 0; j < total_explosion_particles; j++) {
      float timeout = meteors[i].particles_timer;
      if(timeout > 0) {
        float fade_percent = timeout / explosion_particle_total_timeout;
        DrawCircleV(meteors[i].explosion_particles[j].position, 1, (Color){255, 255, 255, 255 * fade_percent});
      }
    }
  }
}

void spawn_meteor(Meteor *meteor) {
  /// @note: padding to keep the location 'inside' the screen boundaries
  int spawn_padding = 30;
  /// @note: offset to keep the location 'outside' the screen boundaries
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
    /// hex numbers just to keep aligned :)
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