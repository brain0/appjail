#include "common.h"
#include "child.h"
#include "cap.h"

#include <sys/prctl.h>
#include <sched.h>
#include <sys/wait.h>

#define STACK_SIZE (1024 * 1024)

int main() {
  char *stack, *stackTop;
  pid_t pid1;
  int status;

  /* Ensure we never elevate privileges again */
  if(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0, 0) == -1)
    errExit("prctl");

  /* Allocate stack for child */
  stack = malloc(STACK_SIZE);
  if (stack == NULL)
    errExit("malloc");
  stackTop = stack + STACK_SIZE;  /* Assume stack grows downward */

  /* Clone a child in an isolated namespace */
  need_cap(CAP_SYS_ADMIN);
  pid1 = clone(child_main, stackTop, CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | SIGCHLD, NULL);
  /* We drop all capabilities from the permitted capability set */
  drop_caps_forever();

  /* clone failed, we are done */
  if (pid1 == -1)
    errExit("clone");

  /* Wait for child the child to terminate
   * If we were interrupted by a signal, wait again
   */
  if(waitpid(pid1, &status, 0) == -1)
      errExit("waitpid");

  if(WIFEXITED(status))
    return WEXITSTATUS(status);
  else
    return EXIT_FAILURE;
}