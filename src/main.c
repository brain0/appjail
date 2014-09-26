#include "common.h"
#include "child.h"
#include "cap.h"
#include "clone.h"
#include "configfile.h"
#include "opts.h"

#include <sys/prctl.h>
#include <sched.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
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

  /* Clone a child in an isolated namespace */
  clone_flags = CLONE_NEWNS | CLONE_NEWPID | SIGCHLD;
  if(opts->unshare_network)
    clone_flags |= CLONE_NEWNET;
  if(!opts->keep_ipc_namespace)
    clone_flags |= CLONE_NEWIPC;
  pid1 = my_clone(clone_flags, child_main, (void*)opts);

  /* clone failed, we are done */
  if (pid1 == -1)
    errExit("clone");

  /* Free some memory */
  free_options(opts);

  /* Wait for the child to terminate */
  if(waitpid(pid1, &status, 0) == -1)
    errExit("waitpid");

  if(WIFEXITED(status))
    return WEXITSTATUS(status);
  else
    return EXIT_FAILURE;
}
