#define _GNU_SOURCE
#include <sys/wait.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/prctl.h>
/*#include <seccomp.h>*/

#define STACK_SIZE (1024 * 1024)

#define errExit(msg) do { \
  perror(msg);\
  exit(EXIT_FAILURE);\
  } while (0)

#define errExitNoErrno(err) do { \
  fprintf(stderr, err "\n");\
  exit(EXIT_FAILURE);\
  } while (0)

static void need_cap(cap_value_t c) {
  cap_t caps;
  caps = cap_get_proc();
  cap_clear_flag(caps, CAP_EFFECTIVE);
  cap_clear_flag(caps, CAP_INHERITABLE);
  cap_set_flag(caps, CAP_EFFECTIVE, 1, &c, CAP_SET);
  if(cap_set_proc(caps) == -1)
    errExit("cap_set_proc");
  cap_free(caps);
}

static void drop_caps() {
  cap_t caps;
  caps = cap_get_proc();
  cap_clear_flag(caps, CAP_EFFECTIVE);
  cap_clear_flag(caps, CAP_INHERITABLE);
  if(cap_set_proc(caps) == -1)
    errExit("cap_set_proc");
  cap_free(caps);
}

static void drop_caps_forever() {
  cap_t caps;
  caps = cap_init();
  if(cap_set_proc(caps) == -1)
    errExit("cap_set_proc");
  cap_free(caps);
}

static int cap_mount(const char *source, const char *target,
              const char *filesystemtype, unsigned long mountflags,
              const void *data) {
  int r;

  need_cap(CAP_SYS_ADMIN);
  r = mount(source, target, filesystemtype, mountflags, data);
  drop_caps();

  return r;
}

static int cap_umount2(const char *target, int flags) {
  int r;

  need_cap(CAP_SYS_ADMIN);
  r = umount2(target, flags);
  drop_caps();

  return r;
}

/*void limit_syscalls() {
  scmp_filter_ctx ctx;
  int i;

  ctx = seccomp_init(SCMP_ACT_ALLOW);
  if(ctx == NULL)
    errExitNoErrno("Error during seccomp_init().");

  if(seccomp_rule_add(ctx, SCMP_ACT_ERRNO(ENOSYS), SCMP_SYS(syslog), 0) < 0)
    errExitNoErrno("Error during seccomp_rule_add() (1).");

  for(i=0; i<=PF_MAX; ++i)
    if(i != PF_LOCAL && i != PF_UNIX && i != PF_INET && i != PF_INET6)
      if(seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EAFNOSUPPORT), SCMP_SYS(socket), 1, SCMP_CMP(0, SCMP_CMP_EQ, i)) < 0)
        errExitNoErrno("Error during seccomp_rule_add() (2).");

  if(seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EINVAL), SCMP_SYS(perf_event_open), 0) < 0)
    errExitNoErrno("Error during seccomp_rule_add() (1).");

  if(seccomp_load(ctx) < 0)
    errExitNoErrno("Error during seccomp_load().");

  seccomp_release(ctx);
}*/

static void make_mounts_private() {
  /* Make our mount a slave of the host - this will make sure all
   * new mounts propagate from the host, but our mounts do not
   * propagate to the host
   */
  if( cap_mount("none", "/", "none", MS_REC | MS_PRIVATE, NULL) == -1)
    errExit("mount --make-rprivate /");
}

static void setup_proc() {
  /* Mount our own local /proc - we have our own PID namespace,
   * so this doesn't give away information regarding the host.
   *
   * The host's /proc/mounts is still visible, sadly.
   */
  if( cap_umount2("/proc", MNT_DETACH) == -1 && errno != EINVAL)
    errExit("umount /proc");
  if( cap_mount("proc", "/proc", "proc", 0, NULL) == -1)
    errExit("mount -t proc proc /proc");
}

static void setup_path(const char *tmpdir, const char *name, const char *path, mode_t mode) {
  char p[PATH_MAX];

  snprintf(p, PATH_MAX-1, "%s/%s", tmpdir, name);
  if( mkdir(p, mode) == -1 )
    errExit("mkdir");
  if( chmod(p, mode) == -1 )
    errExit("chmod");
  if( cap_umount2(path, MNT_DETACH) == -1 && errno != EINVAL)
    errExit("umount2");
  if( cap_mount(p, path, NULL, MS_BIND, NULL) == -1 )
    errExit("mount --bind");
}

static void setup_home_directory() {
  struct passwd *pw;
  char dir[PATH_MAX];

  errno = 0;
  if((pw = getpwuid(getuid())) == NULL)
    errExit("getpwuid");
  snprintf(dir, PATH_MAX-1, "/home/%s", pw->pw_name);
  if(mkdir(dir, 0755) == -1)
    errExit("mkdir");
  if(setenv("HOME", dir, 1) == -1)
    errExit("setenv");
  if(chdir(dir) == -1)
    errExit("chdir");  
}

static int child_main(void *arg) {
  char tmpdir[PATH_MAX];

  drop_caps();
  /* Change the directory to / */
  if(chdir("/") == -1)
    errExit("chdir()");

  make_mounts_private();
  setup_proc();

  /* move /tmp somewhere else */
  if( cap_mount("/tmp", "/var/empty", NULL, MS_MOVE, NULL) == -1 )
    errExit("mount --move /tmp /var/empty");
  /* Create temporary directory */
  strncpy(tmpdir, "/var/empty/appjail-XXXXXX", PATH_MAX);
  if( mkdtemp(tmpdir) == NULL )
    errExit("mkdtemp");

  setup_path(tmpdir, "tmp", "/tmp", 01777);
  setup_path(tmpdir, "vartmp", "/var/tmp", 01777);
  setup_path(tmpdir, "home", "/home", 0755);

  if( cap_umount2("/var/empty", MNT_DETACH) == -1 && errno != EINVAL)
    errExit("umount /var/empty");

  setup_home_directory();

  /* We drop all capabilities from the permitted capability set */
  drop_caps_forever();
  /* Disallow certain unwanted system calls */
  /*limit_syscalls();*/
  execlp("/bin/bash", "/bin/bash", NULL);
  /* We should never be here */
  errExit("execlp");
}

int main() {
  char *stack, *stackTop;
  pid_t pid1;
  int status;

  /* Ensure we never elevate privileges again */
  if(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0, 0) == -1)
    errExit("prctl");

  /* Allocate stack for child */
  stack = malloc(STACK_SIZE);
  if (stack == NULL)
    errExit("malloc");
  stackTop = stack + STACK_SIZE;  /* Assume stack grows downward */

  /* Clone a child in an isolated namespace */
  need_cap(CAP_SYS_ADMIN);
  pid1 = clone(child_main, stackTop, CLONE_NEWIPC | CLONE_NEWNS | CLONE_NEWPID | SIGCHLD, NULL);
  /* We drop all capabilities from the permitted capability set */
  drop_caps_forever();

  /* clone failed, we are done */
  if (pid1 == -1)
    errExit("clone");

  /* Wait for child the child to terminate
   * If we were interrupted by a signal, wait again
   */
  if(waitpid(pid1, &status, 0) == -1)
      errExit("waitpid");

  if(WIFEXITED(status))
    return WEXITSTATUS(status);
  else
    return EXIT_FAILURE;
}
