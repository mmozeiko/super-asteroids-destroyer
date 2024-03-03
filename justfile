just:
  mkdir -p build && clang -Wshadow -Werror main.c -o build/main -lraylib -lm && ./build/main