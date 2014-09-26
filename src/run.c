#include "run.h"
#include "cap.h"
#include "path.h"
#include <limits.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void setup_run(const appjail_options *opts) {
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
