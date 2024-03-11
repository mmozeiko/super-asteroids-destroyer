typedef unsigned long long score_t;
score_t score = 0;
int meteor_score = 10;
int velocity_score = 70;
const char* save_file = "highscore.bin";

void update_score_meteor() {
  score += meteor_score;
}

void save_highscore() {
  TraceLog(LOG_WARNING, "Calling save highscore");
  if(FileExists(save_file)) {
    if(SaveFileData(save_file, &score, sizeof(score))) {
      TraceLog(LOG_WARNING, "Save file success - exist");
    } else {
      TraceLog(LOG_WARNING, "Save file error - exist");
    }
  } else {
    if(SaveFileData(save_file, &score, sizeof(score))) {
      TraceLog(LOG_WARNING, "Save file success");
    } else {
      TraceLog(LOG_WARNING, "Save file error");
    }
  }
}

score_t load_highscore() {
  if(FileExists(save_file)) {
    int size = sizeof(score);
    unsigned char* raw_highscore = LoadFileData(save_file, &size);
    score_t loaded_highscore = *(long*)raw_highscore;
    UnloadFileData(raw_highscore);
    return loaded_highscore;
  } else return 0;
}