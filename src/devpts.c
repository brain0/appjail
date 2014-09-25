#include "devpts.h"
#include "cap.h"
#include "mounts.h"
#include <sys/mount.h>

void setup_devpts() {
  unmount_directory("/dev/pts");
  if( cap_mount("devpts", "/dev/pts", "devpts", 0, "newinstance,gid=5,mode=620,ptmxmode=0666") == -1)
    errExit("mount devpts");
  if( cap_mount("/dev/pts/ptmx", "/dev/ptmx", NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind");
}
