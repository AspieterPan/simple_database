#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_null.h>
#include <sys/_types/_ssize_t.h>

#include "../include/input_buffer.h"

void print_prompt() { printf("db > "); }
void read_input(InputBuffer *input_buffer) {
  ssize_t bytes_read =
      getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
  if (bytes_read <= 0) {
    printf("Error reading input\n");
    exit(EXIT_FAILURE);
  }

  // Ignore trailing newline
  input_buffer->buffer_length = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1] = 0;
}

int main(int argc, char *argv[]) {
  InputBuffer *input_buffer = new_input_buffer();
  while (true) {
    print_prompt();
    read_input(input_buffer);

    if (strcmp(input_buffer->buffer, ".exit") == 0) {
      close_input_buffer(input_buffer);
      exit(EXIT_SUCCESS);
    } else {
      printf("Unrecognized command: %s\n", input_buffer->buffer);
    }
  }

  return 0;
}