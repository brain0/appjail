#include "path.h"
#include "common.h"
#include "mounts.h"
#include "cap.h"

#include <limits.h>
#include <stdio.h>
#include <sys/mount.h>

void setup_path(const char *name, const char *path, mode_t mode) {
  char p[PATH_MAX];

  snprintf(p, PATH_MAX-1, "./%s", name);
  if( mkdir(p, mode) == -1 )
    errExit("mkdir");
  if( chmod(p, mode) == -1 )
    errExit("chmod");
  unmount_directory(path);
  if( cap_mount(p, path, NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind");
  if( cap_mount(NULL, path, NULL, MS_PRIVATE, NULL) == -1 )
    errExit("mount --make-private");
}
