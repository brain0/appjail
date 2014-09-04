#include "command.h"
#include <unistd.h>
#include <sys/wait.h>
#include "cap.h"

static int child(const char *file, char *const argv[]) {
  /* Should be unnecessary, but let's still do it */
  drop_caps_forever();
  execvp(file, argv);
  return -1;
}

static int parent(pid_t childpid) {
  int status;

  waitpid(childpid, &status, 0);
  return WEXITSTATUS(status);
}

int run_command(const char *file, char *const argv[]) {
  pid_t childpid;
  
  childpid = fork();
  switch(childpid) {
    case -1:
      return -1;
    case 0:
      return child(file, argv);
    default:
      return parent(childpid);
  }
}
