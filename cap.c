#include "common.h"
#include "cap.h"
#include <sys/mount.h>

void need_cap(cap_value_t c) {
  cap_t caps;
  caps = cap_get_proc();
  cap_clear_flag(caps, CAP_EFFECTIVE);
  cap_clear_flag(caps, CAP_INHERITABLE);
  cap_set_flag(caps, CAP_EFFECTIVE, 1, &c, CAP_SET);
  if(cap_set_proc(caps) == -1)
    errExit("cap_set_proc");
  cap_free(caps);
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
