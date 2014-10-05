#include "redirect.h"
#include "common.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void redirect_to_dev_null() {
  int fd;

  if((fd = open("/dev/null", O_RDWR)) == -1)
    errExit("open(/dev/null)");
  close(0);
  close(1);
  close(2);
  dup2(fd, 0);
  dup2(fd, 1);
  dup2(fd, 2);
  close(fd);
}
