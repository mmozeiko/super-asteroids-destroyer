Camera2D camera = {};
float current_shake_strength = 0;
float noise_time_acc = 0;

float shake_strength = 60 * 0.4;
float shake_decay_rate = 3;
float shake_speed = 30;

float noise_2d(float x, float y);

void init_camera() {
  camera.target = screen_center;
  camera.zoom = 1;
}

void update_camera(float dt) {
  current_shake_strength = Lerp(current_shake_strength, 0, shake_decay_rate * dt);
  noise_time_acc += dt * shake_speed;
  camera.offset.x = screen_center.x + noise_2d(1,   noise_time_acc) * current_shake_strength;
  camera.offset.y = screen_center.y + noise_2d(100, noise_time_acc) * current_shake_strength;
}

void shake_camera() {
  current_shake_strength = shake_strength;
}

float noise_2d(float x, float y) {
  float z = 0, lacunarity = 2, gain = 0.5, octaves = 3;
  return stb_perlin_fbm_noise3(x, y, z, lacunarity, gain, octaves);
}