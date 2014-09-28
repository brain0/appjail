#include "mask.h"
#include "cap.h"
#include "list_helpers.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void mask_directory(const char *dir) {
  struct stat st;

  if( stat(dir, &st) == -1 )
    return;
  if( !S_ISDIR(st.st_mode) )
    return;
  if( cap_mount("mask", dir, "tmpfs", MS_NODEV | MS_NOSUID, "size=1,mode=0755,uid=0,gid=0") == -1 )
    errExit("mount -t tmpfs -o size=1,mode=0755,uid=0,gid=0 mask DIR");
}

void mask_directories(appjail_options *opts) {
  strlist_node *i;

  for(i = strlist_first(opts->mask_directories); i != NULL; i = strlist_next(i))
    if(!has_path(opts->mask_directories, strlist_val(i), HAS_STRICT_PARENT_OF_NEEDLE))
      mask_directory(strlist_val(i));
}
