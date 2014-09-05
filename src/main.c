#include "common.h"
#include "child.h"
#include "cap.h"
#include "opts.h"
#include "configfile.h"

#include <sys/prctl.h>
#include <sched.h>
#include <sys/wait.h>

#define STACK_SIZE (1024 * 1024)

int main(int argc, char *argv[]) {
  char *stack, *stackTop;
  pid_t pid1;
  int status;
  int clone_flags;
  appjail_options *opts;
  appjail_config *config;

  /* Drop all privileges we might accidentally have */
  drop_caps();

  /* Parse configuration */
  config = parse_config();
  if(config == NULL)
    exit(EXIT_FAILURE);
  /* Parse command line */
  opts = parse_options(argc, argv, config);
  free_config(config);

  if(!opts->allow_new_privs) {
    /* Ensure we never elevate privileges again */
    if(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0, 0) == -1)
      errExit("prctl");
  }

  /* Allocate stack for child */
  stack = malloc(STACK_SIZE);
  if (stack == NULL)
    errExit("malloc");
  stackTop = stack + STACK_SIZE;  /* Assume stack grows downward */

  /* Clone a child in an isolated namespace */
  need_cap(CAP_SYS_ADMIN);
  clone_flags = CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | SIGCHLD;
  if(opts->unshare_network)
    clone_flags |= CLONE_NEWNET;
  pid1 = clone(child_main, stackTop, clone_flags, (void*)opts);
  /* We drop all capabilities from the permitted capability set */
  drop_caps_forever();

  /* clone failed, we are done */
  if (pid1 == -1)
    errExit("clone");

  /* Free some memory */
  free_options(opts);
  free(stack);

  /* Wait for child the child to terminate */
  if(waitpid(pid1, &status, 0) == -1)
    errExit("waitpid");

  if(WIFEXITED(status))
    return WEXITSTATUS(status);
  else
    return EXIT_FAILURE;
}
