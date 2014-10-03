#include "command.h"
#include "cap.h"
#include <unistd.h>
#include <sys/wait.h>

static int child(const char *file, char *const argv[]) {
  /* Should be unnecessary, but let's still do it */
  drop_caps_forever();
  execvp(file, argv);
  return EXIT_FAILURE;
}

static int parent(pid_t childpid, bool wait) {
  int status;

  if(!wait)
    return EXIT_SUCCESS;

  waitpid(childpid, &status, 0);
  if(WIFEXITED(status))
    return WEXITSTATUS(status);
  else
    return EXIT_FAILURE;
}

int run_command(const char *file, char *const argv[], bool wait) {
  pid_t childpid;
  
  childpid = fork();
  switch(childpid) {
    case -1:
      return EXIT_FAILURE;
    case 0:
      return child(file, argv);
    default:
      return parent(childpid, wait);
  }
}
