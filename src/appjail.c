#include "appjail.h"

#include "common.h"
#include "child.h"
#include "cap.h"
#include "clone.h"
#include "configfile.h"
#include "opts.h"
#include "wait.h"

#include <fcntl.h>
#include <sys/prctl.h>
#include <sched.h>
#include <sys/signalfd.h>
#include <signal.h>

int appjail_main(int argc, char *argv[]) {
  pid_t pid1;
  int clone_flags;
  appjail_options *opts;
  appjail_config *config;
  child_options chldopts;
  /* signalfd */
  sigset_t mask, oldmask;
  int sfd;
  /* pipes */
  int pipefds[2];

  /* Initialize the capability handling. Drop all privileges
   * we might accidentally have and only set the permitted
   * capabilities we will need later.
   */
  init_caps();

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
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  if( sigprocmask(SIG_BLOCK, &mask, &oldmask) == -1 )
    errExit("sigprocmask");
  if( (sfd = signalfd(-1, &mask, SFD_CLOEXEC)) == -1 )
    errExit("signalfd");

  /* set up the pipe */
  if( pipe2(pipefds, O_CLOEXEC) == -1)
    errExit("pipe");
  opts->pipefd = pipefds[1];

  /* Clone a child in an isolated namespace */
  clone_flags = CLONE_NEWNS | CLONE_NEWPID | SIGCHLD;
  if(opts->unshare_network)
    clone_flags |= CLONE_NEWNET;
  if(!opts->keep_ipc_namespace)
    clone_flags |= CLONE_NEWIPC;

  chldopts.sfd = sfd;
  chldopts.old_sigmask = &oldmask;
  chldopts.daemonize = opts->daemonize;

  pid1 = launch_child(clone_flags, &chldopts, child_main, (void*)opts);

  /* clone failed, we are done */
  if (pid1 == -1)
    errExit("launch_child");

  close(pipefds[1]);

  /* Free some memory */
  free_options(opts);

  wait_for_child(pid1, sfd, chldopts.daemonize, pipefds[0]);
  return EXIT_FAILURE;
}
