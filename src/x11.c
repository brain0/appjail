#include "x11.h"
#include "common.h"
#include "cap.h"
#include "command.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <limits.h>

#define TIMEOUT_LEN 20

void get_x11(const appjail_options *opts) {
  char *cmd_argv[10];
  char timeout[TIMEOUT_LEN];
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

  snprintf(timeout, TIMEOUT_LEN-1, "%u", opts->x11_timeout);

  cmd_argv[0] = "xauth";
  cmd_argv[1] = "-f";
  cmd_argv[2] = APPJAIL_SWAPDIR "/Xauthority";
  cmd_argv[3] = "generate";
  cmd_argv[4] = display;
  cmd_argv[5] = "MIT-MAGIC-COOKIE-1";
  if( opts->x11_trusted )
    cmd_argv[6] = "trusted";
  else
    cmd_argv[6] = "untrusted";
  cmd_argv[7] = "timeout";
  cmd_argv[8] = timeout;
  cmd_argv[9] = NULL;
  // Create an empty file to silence an xauth error message
  if( (fd = open(cmd_argv[2], O_CREAT, 0600)) != -1)
    close(fd);
  if( run_command(cmd_argv[0], cmd_argv) != EXIT_SUCCESS )
    errExit("xauth");
}

void setup_x11() {
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

