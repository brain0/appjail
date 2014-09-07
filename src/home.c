#include "common.h"
#include "home.h"
#include "cap.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mount.h>

void get_home_directory(const char *homedir) {
  struct stat st;

  if(homedir == NULL)
    return;
  if(lstat(homedir, &st) == -1)
    errExit("Could not stat() home directory");
  if(!S_ISDIR(st.st_mode))
    errExitNoErrno("Home directory is not a directory.");
  if(access(homedir, R_OK | W_OK | X_OK) != 0)
    errExit("Insufficient permissions for home directory");
  if(mkdir("./homedir", 0755) == -1)
    errExit("mkdir");
  if(cap_mount(homedir, "./homedir", NULL, MS_BIND | MS_REC, NULL) == -1)
    errExit("mount --bind");
}

void setup_home_directory(const char *user) {
  char dir[PATH_MAX];
  struct stat st;

  snprintf(dir, PATH_MAX-1, "/home/%s", user);
  if(mkdir(dir, 0755) == -1)
    errExit("mkdir");
  if(setenv("HOME", dir, 1) == -1)
    errExit("setenv");
  if(lstat("./homedir", &st) != -1 && S_ISDIR(st.st_mode)) {
    if(cap_mount("./homedir", dir, NULL, MS_MOVE, NULL) == -1)
      errExit("mount --move");
    rmdir("./homedir");
  }
  if(chdir(dir) == -1)
    errExit("chdir");
}
