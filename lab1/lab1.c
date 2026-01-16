#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {

  char *line = NULL;
  size_t n = 0;

  printf("Please enter some text: \n");

  if (getline(&line, &n, stdin) != -1) {
    printf("Got: %s\n", line);
    printf("Tokens:\n");
    char *ret = NULL;
    char *saveptr = NULL;
    char *str = line;
    while ((ret = strtok_r(str, " ", &saveptr))) {
      printf("%s\n", ret);
      str = NULL;
    }
  } else {
    printf("Failure reading line!\n");
  }
  return 0;
}
