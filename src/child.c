#include "common.h"
#include "cap.h"
#include "child.h"
#include "opts.h"
#include "home.h"
#include "mounts.h"
#include "command.h"
#include "network.h"
#include "tty.h"
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
  int fd;

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
  // Create an empty file to silence an xauth error message
  if( (fd = open(cmd_argv[2], O_CREAT, 0600)) != -1)
    close(fd);
  if( run_command(cmd_argv[0], cmd_argv) != EXIT_SUCCESS )
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
  if( run_command(cmd_argv[0], cmd_argv) != EXIT_SUCCESS )
    errExit("mv");
}

static void setup_path(const char *name, const char *path, mode_t mode) {
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

static void setup_devpts() {
  unmount_directory("/dev/pts");
  if( cap_mount("devpts", "/dev/pts", "devpts", 0, "newinstance,gid=5,mode=620,ptmxmode=0666") == -1)
    errExit("mount devpts");
  if( cap_mount("/dev/pts/ptmx", "/dev/ptmx", NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind");
}

static void setup_run(const appjail_options *opts) {
  char path[PATH_MAX], mediapath[PATH_MAX];
  struct stat st;
  bool bind_media = false;

  if( opts->run_mode != RUN_HOST ) {
    snprintf(path, PATH_MAX-1, "/run/user/%d", opts->uid);
    if( opts->run_mode == RUN_USER ) {
      if( mkdir("runuser", 0755) == -1 )
        errExit("mkdir");
      if( cap_mount(path, "runuser", NULL, MS_BIND | MS_REC, NULL) == -1 )
        errExit("mount --rbind /run/user/UID runuser");
    }

    if( opts->bind_run_media ) {
      snprintf(mediapath, PATH_MAX-1, "/run/media/%s", opts->user);
      if(stat(mediapath, &st) == 0) {
        bind_media = true;
        if( mkdir("runmedia", 0755) == -1 )
          errExit("mkdir");
        if( cap_mount(mediapath, "runmedia", NULL, MS_BIND | MS_REC, NULL) == -1 )
          errExit("mount --rbind /run/media/USER runmedia");
      }
      else
        fprintf(stderr, "Media directory does not exist yet, it will be unavailable inside the jail.\n");
    }

    setup_path("run", "/run", 0755);
    if( mkdir("/run/user", 0755) == -1 )
      errExit("mkdir /run/user");
    if( mkdir(path, 0700) == -1 )
      errExit("mkdir /run/user/UID");
    cap_chown("/run/user", 0, 0);
    if( opts->run_mode == RUN_USER )
      if( cap_mount("runuser", path, NULL, MS_MOVE, NULL ) == -1 )
        errExit("mount --move runuser /run/user/UID");

    if(bind_media) {
      if( mkdir("/run/media", 0755) )
        errExit("mkdir");
      if( mkdir(mediapath, 0755) )
        errExit("mkdir");
      cap_chown("/run/media", 0, 0);
      if( cap_mount("runmedia", mediapath, NULL, MS_MOVE, NULL ) == -1 )
        errExit("mount --move runmedia /run/media/USER");
    }

    cap_chown("/run", 0, 0);
  }
}

int child_main(void *arg) {
  appjail_options *opts = (appjail_options*)arg;

  drop_caps();
  init_libmount();
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
  /* set up /run */
  setup_run(opts);
  /* set up home directory using the one we bound earlier
   * WARNING: We change the current directory from APPJAIL_SWAPDIR to the home directory */
  setup_home_directory(opts->user);
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
