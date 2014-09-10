#include "tty.h"
#include "cap.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mount.h>

void get_tty(appjail_options *opts) {
  const char *console;
  int fd;

  opts->setup_tty = isatty(0);
  
  if(opts->setup_tty) {
    /* Get name of the current TTY */
    if( (console = ttyname(0)) == NULL )
      errExit("ttyname()");
    /* create a dummy file to mount to */
    if((fd = open("console", O_CREAT|O_RDWR, 0)) == -1)
      errExit("open()");
    close(fd);
    /* Make the current TTY accessible in APPJAIL_SWAPDIR/console */
    if( cap_mount(console, "console", NULL, MS_BIND, NULL) == -1)
      errExit("mount --bind $TTY " APPJAIL_SWAPDIR "/console");
    /* Make the console bind private */
    if( cap_mount(NULL, "console", NULL, MS_PRIVATE, NULL) == -1)
      errExit("mount --make-private " APPJAIL_SWAPDIR "/console");
  }
}

void setup_tty(const appjail_options *opts) {
  int fd;

  if(opts->setup_tty) {
    if( cap_mount("console", "/dev/console", NULL, MS_MOVE, NULL) == -1)
      errExit("mount --move " APPJAIL_SWAPDIR "/console /dev/console");
    unlink("console");

    /* The current TTY is now accessible under /dev/console,
    * however, the original device (like /dev/pts/0) will not
    * be accessible in the container. Reopen /dev/console as our
    * standard input, output and error.
    */
    if((fd = open("/dev/console", O_RDWR)) == -1)
      errExit("open(/dev/console)");
    close(0);
    close(1);
    close(2);
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    close(fd);
  }
}