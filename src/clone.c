#define _GNU_SOURCE
#include "common.h"
#include "clone.h"
#include "cap.h"
#include <stdlib.h>
#include <sys/syscall.h>

/* glibc's clone() wrapper requires that you pass a non-NULL argument
 * for the child stack. This is unnecessary in our case and complicates
 * things. We use the system call directly.
 */
static pid_t clone1(int flags) {
  #if defined(__s390__) || defined(__s390x__) || defined(__cris__)
  return syscall(SYS_clone, NULL, flags);
  #else
  return syscall(SYS_clone, flags, NULL);
  #endif
}

static void close_fd(int *fd) {
  /* Close *fd if necessary */
  if( *fd != -1 ) {
    close(*fd);
    *fd = -1;
  }
}

static void restore_sigmask( sigset_t **mask ) {
  /* Restore the correct signal mask */
  if( *mask != NULL ) {
    if( sigprocmask(SIG_SETMASK, *mask, NULL) == -1 )
      errExit("sigprocmask");
    *mask = NULL;
  }
}

pid_t launch_child(int flags, child_options *chldopts, int (*fn)(void *), void *arg) {
  pid_t ret;

  need_cap(CAP_SYS_ADMIN);
  ret = clone1(flags);

  switch(ret) {
    case 0:
      /* Drop all capabilities from the effective capability set */
      drop_caps();
      /* Do some cleanup in the child */
      close_fd(&(chldopts->sfd));
      restore_sigmask(&(chldopts->old_sigmask));
      /* Call the function */
      fn(arg);
      /* The last call should not have returned */
      exit(EXIT_FAILURE);
    default:
      /* Drop all capabilities from the permitted capability set */
      drop_caps_forever();
      return ret;
  }
}
