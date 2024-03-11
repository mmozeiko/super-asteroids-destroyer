just:
  mkdir -p build && clang -g -Wshadow -Werror -fstack-protector-all -fsanitize=address,undefined -fsanitize-trap=all main.c -o build/main -lraylib -lm && ./build/main