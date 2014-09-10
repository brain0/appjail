#include "cap.h"
#include <sys/mount.h>

bool want_cap(cap_value_t c) {
  bool ret;
  cap_t caps;
  caps = cap_get_proc();
  cap_clear_flag(caps, CAP_EFFECTIVE);
  cap_clear_flag(caps, CAP_INHERITABLE);
  cap_set_flag(caps, CAP_EFFECTIVE, 1, &c, CAP_SET);
  ret = cap_set_proc(caps) != -1;
  cap_free(caps);

  return ret;
}

void need_cap(cap_value_t c) {
  if(!want_cap(c))
    errExit("cap_set_proc");
}

void drop_caps() {
  cap_t caps;
  caps = cap_get_proc();
  cap_clear_flag(caps, CAP_EFFECTIVE);
  cap_clear_flag(caps, CAP_INHERITABLE);
  if(cap_set_proc(caps) == -1)
    errExit("cap_set_proc");
  cap_free(caps);
}

void drop_caps_forever() {
  cap_t caps;
  caps = cap_init();
  if(cap_set_proc(caps) == -1)
    errExit("cap_set_proc");
  cap_free(caps);
}

int cap_mount(const char *source, const char *target,
              const char *filesystemtype, unsigned long mountflags,
              const void *data) {
  int r;

  need_cap(CAP_SYS_ADMIN);
  r = mount(source, target, filesystemtype, mountflags, data);
  drop_caps();

  return r;
}

int cap_umount2(const char *target, int flags) {
  int r;

  need_cap(CAP_SYS_ADMIN);
  r = umount2(target, flags);
  drop_caps();

  return r;
}

int cap_chown(const char *path, uid_t owner, gid_t group) {
  static bool warned = false;
  int r;

  if(want_cap(CAP_CHOWN)) {
    r = chown(path, owner, group);
    drop_caps();
  }
  else {
    r = -1;
    if(!warned) {
      fprintf(stderr, "The process is missing CAP_CHOWN capability.\n"
                      "Some directories will be owned by the user although they should be owned by root.\n");
      warned = true;
    }
  }

  return r;
}
