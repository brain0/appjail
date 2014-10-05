#include "wait.h"
#include "common.h"

#include <sys/select.h>
#include <sys/signalfd.h>
#include <sys/wait.h>

static void handle_signalfd(int sfd, pid_t pid1) {
  struct signalfd_siginfo fdsi;
  pid_t spid;
  size_t s;
  int status;

  s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
  if (s != sizeof(struct signalfd_siginfo))
    errExit("read");

  if(fdsi.ssi_signo == SIGCHLD)
    while((spid = waitpid(-1, &status, WNOHANG)) > 0)
      if(spid == pid1) {
        if( WIFEXITED(status) )
          exit( WEXITSTATUS(status) );
        else
          exit( EXIT_FAILURE );
      }
}

void wait_for_child(pid_t pid1, int sfd) {
  int nfds = -1;
  fd_set rfd;

  while(true) {
    nfds = -1;
    FD_ZERO(&rfd);
    FD_SET(sfd, &rfd);
    if(sfd >= nfds)
      nfds = sfd + 1;
    if( select(nfds, &rfd, NULL, NULL, NULL) < 0 )
      errExit("select");

    if( FD_ISSET(sfd, &rfd) )
      handle_signalfd(sfd, pid1);
  }
}
