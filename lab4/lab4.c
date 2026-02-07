#define _DEFAULT_SOURCE
#define _ISOC99_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BLOCK_SIZE 128
#define HEAP_SIZE 256
#define BUF_SIZE 64

struct header {
  uint64_t size;
  struct header *next;
};

void handle_error(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

void print_out(char *format, void *data, size_t data_size) {
  char buf[BUF_SIZE];
  ssize_t len = snprintf(buf, BUF_SIZE, format,
                         data_size == sizeof(uint64_t) ? *(uint64_t *)data
                                                       : *(void **)data);
  if (len < 0) {
    handle_error("snprintf");
  }
  write(STDOUT_FILENO, buf, len);
}

void print_block(char *start) {
  for (int i = 0; i < BLOCK_SIZE - sizeof(struct header); ++i) {
    char *addr = (start + sizeof(struct header) + i);
    uint64_t val = (uint64_t)*addr;
    print_out("%lu\n", &val, sizeof(val));
  }
}

void initialize_block(struct header *block, uint64_t size, struct header *next,
                      int fill_value) {
  block->size = size;
  block->next = next;
  memset(block + 1, fill_value, size - sizeof(struct header));
}

int main(void) {
  char *heap_start = sbrk(0);
  if (sbrk(HEAP_SIZE) == (void *)-1) {
    handle_error("sbrk");
  }

  struct header *first_block = (struct header *)heap_start;
  struct header *second_block = (struct header *)(heap_start + BLOCK_SIZE);

  initialize_block(first_block, BLOCK_SIZE, NULL, 0);
  initialize_block(second_block, BLOCK_SIZE, first_block, 1);

  print_out("first block: %p\n", &first_block, sizeof(first_block));
  print_out("second block: %p\n", &second_block, sizeof(second_block));
  print_out("first block size: %lu\n", &first_block->size,
            sizeof(first_block->size));
  print_out("first block next: %p\n", &first_block->next,
            sizeof(first_block->next));
  print_out("second block size: %lu\n", &second_block->size,
            sizeof(second_block->size));
  print_out("second block next: %p\n", &second_block->next,
            sizeof(second_block->next));
  print_block((char *)first_block);
  print_block((char *)second_block);

  return 0;
}
