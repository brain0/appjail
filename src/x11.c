#include "x11.h"
#include "common.h"
#include "cap.h"
#include "command.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <limits.h>

void get_x11() {
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

