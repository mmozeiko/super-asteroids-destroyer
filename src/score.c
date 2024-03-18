typedef unsigned long long score_t;
score_t score = 0;
int meteor_score = 10;
int velocity_score = 70;
const char* save_file = "highscore.bin";

void update_score_meteor() {
  score += meteor_score;
}

void save_highscore() {
  SaveFileData(save_file, &score, sizeof(score));
}

score_t load_highscore() {
  if(FileExists(save_file)) {
    int size = sizeof(score);
    unsigned char* bytes_highscore = LoadFileData(save_file, &size);
    score_t loaded_highscore = *(score_t*)bytes_highscore;
    UnloadFileData(bytes_highscore);
    return loaded_highscore;
  } else return 0;
}