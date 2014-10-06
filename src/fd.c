#include "fd.h"
#include "common.h"
#include "list_helpers.h"
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <unistd.h>

void close_file_descriptors(intlist *keepfds) {
  DIR *dir;
  struct dirent *e;
  int fd, flags;

  dir = opendir("/proc/self/fd");
  if( dir != NULL ) {
    while( (e=readdir(dir)) != NULL )
      if( string_to_integer(&fd, e->d_name) && fd > 2 && !intlist_contains(keepfds, fd)) {
        flags = fcntl(fd, F_GETFD);
        if(flags != -1 && !(flags & FD_CLOEXEC)) {
          flags |= FD_CLOEXEC;
          fcntl(fd, F_SETFD, flags);
        }
      }
    closedir(dir);
  }
}
