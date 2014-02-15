#include "common.h"
#include "cap.h"
#include "child.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

static void make_mounts_private() {
  /* Make our mount a slave of the host - this will make sure all
   * new mounts propagate from the host, but our mounts do not
   * propagate to the host
   */
  if( cap_mount("none", "/", "none", MS_REC | MS_PRIVATE, NULL) == -1)
    errExit("mount --make-rprivate /");
}

static void setup_proc() {
  /* Mount our own local /proc - we have our own PID namespace,
   * so this doesn't give away information regarding the host.
   *
   * The host's /proc/mounts is still visible, sadly.
   */
  if( cap_umount2("/proc", MNT_DETACH) == -1 && errno != EINVAL)
    errExit("umount /proc");
  if( cap_mount("proc", "/proc", "proc", 0, NULL) == -1)
    errExit("mount -t proc proc /proc");
}

static void setup_path(const char *tmpdir, const char *name, const char *path, mode_t mode) {
  char p[PATH_MAX];

  snprintf(p, PATH_MAX-1, "%s/%s", tmpdir, name);
  if( mkdir(p, mode) == -1 )
    errExit("mkdir");
  if( chmod(p, mode) == -1 )
    errExit("chmod");
  if( cap_umount2(path, MNT_DETACH) == -1 && errno != EINVAL)
    errExit("umount2");
  if( cap_mount(p, path, NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind");
}

static void setup_home_directory() {
  struct passwd *pw;
  char dir[PATH_MAX];

  errno = 0;
  if((pw = getpwuid(getuid())) == NULL)
    errExit("getpwuid");
  snprintf(dir, PATH_MAX-1, "/home/%s", pw->pw_name);
  if(mkdir(dir, 0755) == -1)
    errExit("mkdir");
  if(setenv("HOME", dir, 1) == -1)
    errExit("setenv");
  if(chdir(dir) == -1)
    errExit("chdir");  
}

static void setup_tty() {
  const char *console;
  int fd;

  /* Get name of the current TTY */
  if( (console = ttyname(0)) == NULL )
    errExit("ttyname()");
  /* Make the current TTY accessible under /dev/console */
  if( cap_mount(console, "/dev/console", NULL, MS_BIND, NULL) == -1)
    errExit("mount --bind $TTY /dev/console");

  /* The current TTY is now accessible under /dev/console,
   * however, the original device (like /dev/pts/0) will not
   * be accessible in the container. Reopen /dev/console as our
   * standard input, output and error.
   */
  if((fd = open("/dev/console", O_RDWR)) == -1)
    errExit("open(/dev/console)");
  close(0);
  close(1);
  close(2);
  dup2(fd, 0);
  dup2(fd, 1);
  dup2(fd, 2);
  close(fd);
}

static void setup_devpts() {
  if( cap_umount2("/dev/pts", MNT_DETACH) == -1 && errno != EINVAL)
    errExit("umount2");
  if( cap_mount("devpts", "/dev/pts", "devpts", 0, "newinstance,gid=5,mode=620,ptmxmode=0666") == -1)
    errExit("mount devpts");
  if( cap_mount("/dev/pts/ptmx", "/dev/ptmx", NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind");
}

static void setup_shm() {
  if( cap_umount2("/dev/shm", MNT_DETACH) == -1 && errno != EINVAL)
    errExit("umount2");
  if( cap_mount("shm", "/dev/shm", "tmpfs", MS_NODEV | MS_NOSUID, "mode=1777,uid=0,gid=0") == -1)
    errExit("mount shm");
}

int child_main(void *arg) {
  char tmpdir[PATH_MAX];

  drop_caps();
  /* Change the directory to / */
  if(chdir("/") == -1)
    errExit("chdir()");

  make_mounts_private();
  setup_proc();

  /* move /tmp somewhere else */
  if( cap_mount("/tmp", "/var/empty", NULL, MS_MOVE, NULL) == -1 )
    errExit("mount --move /tmp /var/empty");
  /* Create temporary directory */
  strncpy(tmpdir, "/var/empty/appjail-XXXXXX", PATH_MAX-1);
  if( mkdtemp(tmpdir) == NULL )
    errExit("mkdtemp");

  setup_path(tmpdir, "tmp", "/tmp", 01777);
  setup_path(tmpdir, "vartmp", "/var/tmp", 01777);
  setup_path(tmpdir, "home", "/home", 0755);

  if( cap_umount2("/var/empty", MNT_DETACH) == -1 && errno != EINVAL)
    errExit("umount /var/empty");

  setup_tty();
  setup_devpts();
  setup_shm();

  setup_home_directory();

  /* Make some permissions consistent */
  cap_chown("/tmp", 0, 0);
  cap_chown("/var/tmp", 0, 0);
  cap_chown("/home", 0, 0);

  /* We drop all capabilities from the permitted capability set */
  drop_caps_forever();

  execlp("/bin/bash", "/bin/bash", NULL);
  /* We should never be here */
  errExit("execlp");
}
