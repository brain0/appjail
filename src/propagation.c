#include "common.h"
#include "propagation.h"
#include "cap.h"
#include <libmount.h>
#include <string.h>

void set_mount_propagation_slave() {
  if( cap_mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL) == -1)
    errExit("mount --make-rslave /");
}

static int error_cb(struct libmnt_table *tb, const char *filename, int line) {
  fprintf(stderr, "Failed to parse in %s, line %d.\n", filename, line);
  return 1;
}

static void unmount_recursive(struct libmnt_table *t, struct libmnt_fs *r) {
  struct libmnt_iter *i = mnt_new_iter(MNT_ITER_FORWARD);
  struct libmnt_fs *f;

  while(mnt_table_next_child_fs(t, i, r, &f) == 0) {
    unmount_recursive(t, f);
    mnt_unref_fs(f);
  }
  if(cap_umount2(mnt_fs_get_target(r), 0) == -1)
    errExit("umount");

  mnt_free_iter(i);
}

static bool has_beginning_of_path(const char **l, const char *p) {
  const char **i;
  size_t len;

  i = l;
  while(*i != NULL) {
    len = strlen(*i);
    if(!strncmp(*i, p, len))
      if(strlen(p) == len || (strlen(p) > len && p[len] == '/'))
        return true;
    ++i;
  }

  return false;
}

static void unmount_or_make_private(struct libmnt_table *t, struct libmnt_fs *r, appjail_options *opts) {
  const char *path = mnt_fs_get_target(r);

  if(!strcmp(path, APPJAIL_SWAPDIR))
    return;

  if(has_beginning_of_path(opts->special_directories.unmount_directories, path)) {
    unmount_recursive(t, r);
  }
  else {
    struct libmnt_iter *i = mnt_new_iter(MNT_ITER_FORWARD);
    struct libmnt_fs *f;

    if(!has_beginning_of_path(opts->special_directories.shared_directories, path))
      if(cap_mount(NULL, path, NULL, MS_PRIVATE, NULL) == -1)
        errExit("mount --make-private");

    while(mnt_table_next_child_fs(t, i, r, &f) == 0) {
      unmount_or_make_private(t, f, opts);
      mnt_unref_fs(f);
    }
    mnt_free_iter(i);
  }
}

void sanitize_mounts(appjail_options *opts) {
  struct libmnt_fs *f;
  struct libmnt_table *t;

  /* libmount setup */
  mnt_init_debug(0);
  t = mnt_new_table();
  mnt_table_set_parser_errcb(t, error_cb);

  /* make / a private monut */
  if( cap_mount(NULL, "/", NULL, MS_PRIVATE, NULL) == -1)
    errExit("mount --make-private /");

  /* parse /proc/self/mountinfo */
  mnt_table_parse_file(t, "/proc/self/mountinfo");

  /* First, handle /proc - umount /proc recursively */
  f = mnt_table_find_mountpoint(t, "/proc", MNT_ITER_FORWARD);
  if(f != NULL) {
    unmount_recursive(t, f);
    mnt_unref_fs(f);
  }

  /* Mount our own local /proc - we have our own PID namespace. */
  if( cap_mount("proc", "/proc", "proc", 0, NULL) == -1)
    errExit("mount -t proc proc /proc");

  /* re-read /proc/self/mountinfo */
  mnt_table_parse_file(t, "/proc/self/mountinfo");

  f = NULL;
  if((mnt_table_get_root_fs(t, &f)) != 0)
    errExitNoErrno("Error while processing mountinfo");
  unmount_or_make_private(t, f, opts);
  mnt_unref_fs(f);

  /* free the libmount data */
  mnt_unref_table(t);
}
