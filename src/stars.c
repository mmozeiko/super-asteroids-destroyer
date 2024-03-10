typedef struct {
  Vector2 position;
  int size;
} Star;

#define total_stars 100
Star stars[total_stars] = {};

void init_stars() {
  for (int i = 0; i < total_stars; i++) {
    stars[i].position = (Vector2){
      GetRandomValue(-1000, screen_width + 1000),
      GetRandomValue(-1000, screen_height + 1000),
    };
    stars[i].size = GetRandomValue(1, 3);
  }
}

void update_stars() {
  for (int i = 0; i < total_stars; i++) {
    stars[i].position.x = Wrap(++stars[i].position.x, 0, screen_width);
    stars[i].position.y = Wrap(++stars[i].position.y, 0, screen_height);
  }
}

void draw_stars() {
  for (int i = 0; i < total_stars; i++) {
    DrawCircleV(stars[i].position, stars[i].size, GRAY);
  }
}