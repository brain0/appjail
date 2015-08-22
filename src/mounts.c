#include "common.h"
#include "list_helpers.h"
#include "mounts.h"
#include "cap.h"
#include <sys/mount.h>
#include <string.h>
#include <libmount.h>

void init_libmount() {
  /* libmount setup */
  mnt_init_debug(0);
}

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

static void unmount_or_make_private(struct libmnt_table *t, struct libmnt_fs *r, appjail_options *opts) {
  const char *path = mnt_fs_get_target(r);

  if(has_path(opts->special_mounts, path, HAS_EXACT_PATH))
    return;

  if(
        strcmp(path, "/")
     && !has_path(opts->keep_mounts, path, HAS_CHILD_OF_NEEDLE)
     && !has_path(opts->keep_mounts_full, path, HAS_CHILD_OF_NEEDLE)
     && !has_path(opts->keep_mounts_full, path, HAS_PARENT_OF_NEEDLE)
     && !has_path(opts->special_mounts, path, HAS_CHILD_OF_NEEDLE)
    ) {
    unmount_recursive(t, r);
  }
  else {
    struct libmnt_iter *i = mnt_new_iter(MNT_ITER_FORWARD);
    struct libmnt_fs *f;

    if(!has_path(opts->shared_mounts, path, HAS_PARENT_OF_NEEDLE))
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

  t = mnt_new_table();
  mnt_table_set_parser_errcb(t, error_cb);

  /* parse /proc/self/mountinfo */
  mnt_table_parse_file(t, "/proc/self/mountinfo");

  /* First, handle /proc - umount /proc recursively */
  f = mnt_table_find_mountpoint(t, "/proc", MNT_ITER_FORWARD);
  if(f != NULL) {
    if(!strcmp("/proc", mnt_fs_get_target(f)))
      unmount_recursive(t, f);
    mnt_unref_fs(f);
  }

  /* Mount our own local /proc - we have our own PID namespace. */
  if( cap_mount("proc", "/proc", "proc", 0, NULL) == -1)
    errExit("mount -t proc proc /proc");

  /* re-read /proc/self/mountinfo */
  mnt_reset_table(t);
  mnt_table_parse_file(t, "/proc/self/mountinfo");

  f = NULL;
  if((mnt_table_get_root_fs(t, &f)) != 0)
    errExitNoErrno("Error while processing mountinfo");
  unmount_or_make_private(t, f, opts);
  mnt_unref_fs(f);

  /* free the libmount data */
  mnt_unref_table(t);
}

void unmount_directory(const char *path) {
  struct libmnt_fs *f;
  struct libmnt_table *t;

  t = mnt_new_table();
  mnt_table_set_parser_errcb(t, error_cb);

  /* parse /proc/self/mountinfo */
  mnt_table_parse_file(t, "/proc/self/mountinfo");

  /* Unmount path */
  f = mnt_table_find_mountpoint(t, path, MNT_ITER_FORWARD);
  if(f != NULL) {
    if(!strcmp(path, mnt_fs_get_target(f)))
      unmount_recursive(t, f);
    mnt_unref_fs(f);
  }

  /* free the libmount data */
  mnt_unref_table(t);
}

static void make_mount_read_only(struct libmnt_table *t, struct libmnt_fs *r, const appjail_options *opts) {
  const char *path = mnt_fs_get_target(r);
  struct libmnt_iter *i;
  struct libmnt_fs *f;

  if( has_path(opts->special_mounts, path, HAS_EXACT_PATH)
      || !strcmp(path, "/tmp")
      || !strcmp(path, "/var/tmp") )
    return;

  if(cap_mount(NULL, path, NULL, MS_REMOUNT | MS_RDONLY | MS_BIND, NULL) == -1)
    errExit("mount -o remount,ro");

  i = mnt_new_iter(MNT_ITER_FORWARD);
  while(mnt_table_next_child_fs(t, i, r, &f) == 0) {
    make_mount_read_only(t, f, opts);
    mnt_unref_fs(f);
  }
  mnt_free_iter(i);
}

void make_read_only(const appjail_options *opts) {
  struct libmnt_fs *f;
  struct libmnt_table *t;

  t = mnt_new_table();
  mnt_table_set_parser_errcb(t, error_cb);

  /* parse /proc/self/mountinfo */
  mnt_table_parse_file(t, "/proc/self/mountinfo");

  f = NULL;
  if((mnt_table_get_root_fs(t, &f)) != 0)
    errExitNoErrno("Error while processing mountinfo");
  make_mount_read_only(t, f, opts);
  mnt_unref_fs(f);

  /* free the libmount data */
  mnt_unref_table(t);
}
