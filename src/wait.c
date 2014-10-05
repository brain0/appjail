#include "wait.h"
#include "common.h"

#include <sys/select.h>
#include <sys/signalfd.h>
#include <sys/wait.h>

static void handle_signalfd(int sfd, pid_t pid1, bool daemonize, bool child_initialized) {
  struct signalfd_siginfo fdsi;
  pid_t spid;
  size_t s;
  int status;

  s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
  if (s != sizeof(struct signalfd_siginfo))
    errExit("read");

  if(fdsi.ssi_signo == SIGCHLD)
    while((spid = waitpid(-1, &status, WNOHANG)) > 0)
      if(!daemonize && spid == pid1) {
        if(child_initialized) {
          if( WIFEXITED(status) )
            exit( WEXITSTATUS(status) );
          else
            exit( EXIT_FAILURE );
        }
        else {
          fprintf(stderr, APPLICATION_NAME ": Child failed to initialize.\n");
          exit(EXIT_FAILURE);
        }
      }
}


static void handle_pipe(int *pipefd, bool daemonize, bool *child_initialized) {
  size_t s;
  uint8_t u = 0;

  if( *pipefd == -1 )
    return;

  s = read(*pipefd, &u, sizeof(uint8_t));
  if(s == 0) {
    /* end of file, this is an error if the child has
     * not signaled that it finished initializing */
    if( *child_initialized )
      *pipefd = -1;
    else {
      fprintf(stderr, APPLICATION_NAME ": Child failed to initialize.\n");
      exit(EXIT_FAILURE);
    }
  }
  else if(s == sizeof(uint8_t) && u == 1) {
    /* child was successfully initialized */
    fprintf(stderr, APPLICATION_NAME ": Child initialized.\n");
    *child_initialized = true;
    if( daemonize )
      exit(EXIT_SUCCESS);
  }
}

void wait_for_child(pid_t pid1, int sfd, bool daemonize, int pipefd) {
  int nfds = -1;
  bool child_initialized = false;
  fd_set rfd;

  while(true) {
    nfds = -1;
    FD_ZERO(&rfd);
    FD_SET(sfd, &rfd);
    if(sfd >= nfds)
      nfds = sfd + 1;
    if( pipefd != -1 ) {
      FD_SET(pipefd, &rfd);
      if(pipefd >= nfds)
        nfds = pipefd + 1;
    }

    if( select(nfds, &rfd, NULL, NULL, NULL) < 0 )
      errExit("select");

    if( FD_ISSET(sfd, &rfd) )
      handle_signalfd(sfd, pid1, daemonize, child_initialized);

    if( pipefd != -1 && FD_ISSET(pipefd, &rfd) )
      handle_pipe(&pipefd, daemonize, &child_initialized);
  }
}
