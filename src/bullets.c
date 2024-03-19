typedef struct {
  Vector2 position;
  Vector2 velocity;
  float rotation;
} Bullet;

int bullet_radius = 10;
Texture2D bullet_texture;
#define total_bullets 100
Bullet bullets[total_bullets] = {};
int bullet_index = 0;
Sound explosion_sfx;

void reset_bullets();

void init_bullets() {
  explosion_sfx = LoadSound("assets/boom.wav");
  bullet_texture = LoadTexture("assets/bullet.png");
  SetTextureFilter(bullet_texture, TEXTURE_FILTER_BILINEAR);
  reset_bullets();
}

void shoot_bullet(Vector2 position, float rotation) {
  float radians = rotation * DEG2RAD;
  Vector2 direction = {sin(radians), -cos(radians)};
  bullets[bullet_index] = (Bullet){
    (Vector2){position.x, position.y},
    (Vector2){direction.x * 100 * 5, direction.y * 100 * 5},
    rotation,
  };
  bullet_index = bullet_index < total_bullets - 1 ? bullet_index + 1 : 0;
}

void reset_bullet(int index) {
  bullets[index] = (Bullet){
    (Vector2){-30, -30},
    (Vector2){0, 0},
    0,
  };
}

void reset_bullets() {
  for(int i = 0; i < total_bullets; i++) {
    reset_bullet(i);
  }
}

bool is_out_of_bounds(Vector2 position);

void update_bullets(float dt, float slowmotion_factor) {
  for(int i = 0; i < total_bullets; i++) {
    bullets[i].position.x += bullets[i].velocity.x * dt * slowmotion_factor;
    bullets[i].position.y += bullets[i].velocity.y * dt * slowmotion_factor;

    if(is_out_of_bounds(bullets[i].position)) {
      reset_bullet(i);
    }
  }
}

void update_score_meteor();

void bullets_check_collision_with_meteor(Vector2 meteor_center, Meteor* meteor) {
  for(int i = 0; i < total_bullets; i++) {
    if(CheckCollisionCircles(meteor_center, meteor->radius, bullets[i].position, bullet_radius)) {
      shake_camera();
      update_score_meteor();
      PlaySound(explosion_sfx);
      set_explosion_particles(meteor_center, meteor);
      spawn_meteor(meteor);
      reset_bullet(i);
    }
  }
}

void draw_bullets() {
  for(int i = 0; i < total_bullets; i++) {
    Bullet bullet = bullets[i];
    DrawTexturePro(bullet_texture,
      (Rectangle){0, 0, bullet_texture.width, bullet_texture.height},
      (Rectangle){bullet.position.x, bullet.position.y, bullet_texture.width, bullet_texture.height},
      (Vector2){bullet_texture.width / 2.0, bullet_texture.height / 2.0},
      bullet.rotation, WHITE);
  }
}