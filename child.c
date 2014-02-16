#include "common.h"
#include "cap.h"
#include "child.h"
#include "opts.h"
#include "home.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

static void set_mount_propagation() {
  /* Make our mount a slave of the host - this will make sure all
   * new mounts propagate from the host, but our mounts do not
   * propagate to the host
   */
  if( cap_mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL) == -1)
    errExit("mount --make-rslave /");
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

static void setup_path(const char *name, const char *path, mode_t mode) {
  char p[PATH_MAX];

  snprintf(p, PATH_MAX-1, "./%s", name);
  if( mkdir(p, mode) == -1 )
    errExit("mkdir");
  if( chmod(p, mode) == -1 )
    errExit("chmod");
  if( cap_umount2(path, MNT_DETACH) == -1 && errno != EINVAL)
    errExit("umount2");
  if( cap_mount(p, path, NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind");
  if( cap_mount(NULL, path, NULL, MS_PRIVATE, NULL) == -1)
    errExit("mount --make-rprivate");
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
  appjail_options *opts = (appjail_options*)arg;

  drop_caps();
  set_mount_propagation();
  /* Create temporary directory */
  strncpy(tmpdir, "/tmp/appjail-XXXXXX", PATH_MAX-1);
  if( mkdtemp(tmpdir) == NULL )
    errExit("mkdtemp");
  /* Bind the temporary directory to /var/empty
   * This isn't nice, but we need a directory that we won't touch */
  if( cap_mount(tmpdir, "/var/empty", NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind TMPDIR /var/empty");
  /* Change into the temporary directory */
  if(chdir("/var/empty") == -1)
    errExit("chdir()");

  /* Bind the home directory before we change any other mounts */
  get_home_directory(opts->homedir);

  /* set up our private mounts */
  setup_proc();
  setup_path("tmp", "/tmp", 01777);
  setup_path("vartmp", "/var/tmp", 01777);
  setup_path("home", "/home", 0755);

  setup_tty();
  setup_devpts();
  setup_shm();

  setup_home_directory();

  /* unmount our temporary directory */
  if( cap_umount2("/var/empty", MNT_DETACH) == -1 )
    errExit("umount /var/empty");

  /* Make some permissions consistent */
  cap_chown("/tmp", 0, 0);
  cap_chown("/var/tmp", 0, 0);
  cap_chown("/home", 0, 0);

  /* We drop all capabilities from the permitted capability set */
  drop_caps_forever();

  if(opts->argv[0] != NULL) {
    execvp(opts->argv[0], opts->argv);
    errExit("execvp");
  }
  else {
    execl("/bin/sh", "/bin/sh", "-i", NULL);
    errExit("execl");
  }
}
