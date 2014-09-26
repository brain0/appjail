#define _GNU_SOURCE
#include "clone.h"
#include "cap.h"
#include <stdlib.h>
#include <sys/syscall.h>

/* glibc's clone() wrapper requires that you pass a non-NULL argument
 * for the child stack. This is unnecessary in our case and complicates
 * things. We use the system call directly.
 */
pid_t my_clone(int flags, int (*fn)(void *), void *arg) {
  pid_t ret;

  need_cap(CAP_SYS_ADMIN);
  #if defined(__s390__) || defined(__s390x__) || defined(__cris__)
  ret = syscall(SYS_clone, NULL, flags);
  #else
  ret = syscall(SYS_clone, flags, NULL);
  #endif
  switch(ret) {
    case 0:
      drop_caps();
      fn(arg);
      exit(EXIT_FAILURE);
    default:
      /* Drop all capabilities from the permitted capability set */
      drop_caps_forever();
      return ret;
  }
}
