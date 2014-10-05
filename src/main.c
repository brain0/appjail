#include "common.h"
#include "child.h"
#include "cap.h"
#include "clone.h"
#include "configfile.h"
#include "opts.h"
#include "wait.h"

#include <sys/prctl.h>
#include <sched.h>
#include <sys/signalfd.h>
#include <signal.h>

int main(int argc, char *argv[]) {
  pid_t pid1;
  int clone_flags;
  appjail_options *opts;
  appjail_config *config;
  child_options chldopts;
  /* signalfd */
  sigset_t mask, oldmask;
  int sfd;

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

  /* set up signalfd */
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  if( sigprocmask(SIG_BLOCK, &mask, &oldmask) == -1 )
    errExit("sigprocmask");
  if( (sfd = signalfd(-1, &mask, SFD_CLOEXEC)) == -1 )
    errExit("signalfd");

  /* Clone a child in an isolated namespace */
  clone_flags = CLONE_NEWNS | CLONE_NEWPID | SIGCHLD;
  if(opts->unshare_network)
    clone_flags |= CLONE_NEWNET;
  if(!opts->keep_ipc_namespace)
    clone_flags |= CLONE_NEWIPC;

  chldopts.sfd = sfd;
  chldopts.old_sigmask = &oldmask;

  pid1 = launch_child(clone_flags, &chldopts, child_main, (void*)opts);

  /* clone failed, we are done */
  if (pid1 == -1)
    errExit("clone");

  /* Free some memory */
  free_options(opts);

  wait_for_child(pid1, sfd);
  return EXIT_FAILURE;
}
