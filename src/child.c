#include "common.h"
#include "cap.h"
#include "child.h"
#include "opts.h"
#include "home.h"
#include "propagation.h"
#include "command.h"
#include "network.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void get_x11() {
  char *cmd_argv[8];
  char *display;

  if( mkdir(APPJAIL_SWAPDIR "/X11-unix", 0755) == -1 )
    errExit("mkdir");
  if( cap_mount("/tmp/.X11-unix", APPJAIL_SWAPDIR "/X11-unix", NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind");
  if( cap_mount(NULL, APPJAIL_SWAPDIR "/X11-unix", NULL, MS_PRIVATE, NULL) == -1 )
    errExit("mount --make-private");

  display = getenv("DISPLAY");
  if( display == NULL )
    return;

  cmd_argv[0] = "xauth";
  cmd_argv[1] = "-f";
  cmd_argv[2] = APPJAIL_SWAPDIR "/Xauthority";
  cmd_argv[3] = "generate";
  cmd_argv[4] = display;
  cmd_argv[5] = "MIT-MAGIC-COOKIE-1";
  cmd_argv[6] = "trusted";
  cmd_argv[7] = NULL;
  if( run_command(cmd_argv[0], cmd_argv) != 0 )
    errExit("xauth");
}

static void setup_x11() {
  char *home, xauthority[PATH_MAX], *cmd_argv[4];

  if( mkdir("/tmp/.X11-unix", 0755) == -1 )
    errExit("mkdir");
  if( cap_mount(APPJAIL_SWAPDIR "/X11-unix", "/tmp/.X11-unix", NULL, MS_MOVE, NULL) == -1 )
    errExit("mount --move " APPJAIL_SWAPDIR "/X11-unix /tmp/.X11-unix");
  rmdir("X11-unix");

  home = getenv("HOME");
  if( home == NULL )
    return;

  snprintf(xauthority, PATH_MAX-1, "%s/.Xauthority", home);
  cmd_argv[0] = "mv";
  cmd_argv[1] = APPJAIL_SWAPDIR "/Xauthority";
  cmd_argv[2] = xauthority;
  cmd_argv[3] = NULL;
  if( run_command(cmd_argv[0], cmd_argv) == -1 )
    errExit("mv");
}

static void setup_path(const char *name, const char *path, mode_t mode) {
  char p[PATH_MAX];

  snprintf(p, PATH_MAX-1, "./%s", name);
  if( mkdir(p, mode) == -1 )
    errExit("mkdir");
  if( chmod(p, mode) == -1 )
    errExit("chmod");
  if( cap_mount(p, path, NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind");
  if( cap_mount(NULL, path, NULL, MS_PRIVATE, NULL) == -1 )
    errExit("mount --make-private");
}

static void get_tty() {
  const char *console;
  int fd;

  /* Get name of the current TTY */
  if( (console = ttyname(0)) == NULL )
    errExit("ttyname()");
  /* create a dummy file to mount to */
  if((fd = open("console", O_CREAT|O_RDWR, 0)) == -1)
    errExit("open()");
  close(fd);
  /* Make the current TTY accessible in APPJAIL_SWAPDIR/console */
  if( cap_mount(console, "console", NULL, MS_BIND, NULL) == -1)
    errExit("mount --bind $TTY " APPJAIL_SWAPDIR "/console");
  /* Make the console bind private */
  if( cap_mount(NULL, "console", NULL, MS_PRIVATE, NULL) == -1)
    errExit("mount --make-private " APPJAIL_SWAPDIR "/console");
}

static void setup_tty() {
  int fd;

  if( cap_mount("console", "/dev/console", NULL, MS_MOVE, NULL) == -1)
    errExit("mount --move " APPJAIL_SWAPDIR "/console /dev/console");
  unlink("console");

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
  if( cap_mount("devpts", "/dev/pts", "devpts", 0, "newinstance,gid=5,mode=620,ptmxmode=0666") == -1)
    errExit("mount devpts");
  if( cap_mount("/dev/pts/ptmx", "/dev/ptmx", NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind");
}

int child_main(void *arg) {
  appjail_options *opts = (appjail_options*)arg;

  drop_caps();
  /* Set up the private network */
  if(opts->unshare_network)
    if( configure_loopback_interface() != 0 )
      fprintf(stderr, "Unable to configure loopback interface\n");
  /* Make our mount a slave of the host - this will make sure our
   * mounts do not propagate to the host. If we made everything
   * private now, we would lose the ability to keep anything as slave.
   */
  set_mount_propagation_slave();
  /* Mount tmpfs to contain our private data to APPJAIL_SWAPDIR*/
  if( cap_mount("appjail", APPJAIL_SWAPDIR, "tmpfs", MS_NODEV | MS_NOSUID, "") == -1 )
    errExit("mount -t tmpfs appjail " APPJAIL_SWAPDIR);
  /* Change into the temporary directory */
  if(chdir(APPJAIL_SWAPDIR) == -1)
    errExit("chdir()");

  /* Bind directories and files that may disappear */
  get_home_directory(opts->homedir);
  get_tty();
  if(opts->keep_x11)
    /* Get X11 socket directory and xauth data */
    get_x11();

  /* clean up the mounts, making almost everything private */
  sanitize_mounts(opts);

  /* set up our private mounts */
  setup_path("tmp", "/tmp", 01777);
  setup_path("vartmp", "/var/tmp", 01777);
  setup_path("home", "/home", 0755);
  if(!opts->keep_shm)
    setup_path("shm", "/dev/shm", 01777);
  setup_devpts();

  /* set up the tty */
  setup_tty();
  /* set up home directory using the one we bound earlier */
  setup_home_directory();
  if(opts->keep_x11)
    /* Set up X11 socket directory and xauth data */
    setup_x11();

  /* unmount our temporary directory */
  if( cap_umount2(APPJAIL_SWAPDIR, 0) == -1 )
    errExit("umount " APPJAIL_SWAPDIR);

  /* Make some permissions consistent */
  cap_chown("/tmp", 0, 0);
  cap_chown("/var/tmp", 0, 0);
  cap_chown("/home", 0, 0);
  if(!opts->keep_shm)
    cap_chown("/dev/shm", 0, 0);

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
