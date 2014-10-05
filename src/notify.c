#include "notify.h"
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

void signal_mainpid(int pipefd) {
  uint8_t u;
  size_t s;

  u = 1;
  s = write(pipefd, &u, sizeof(uint8_t));
  if (s != sizeof(uint8_t))
    exit(EXIT_FAILURE);
  close(pipefd);
}
