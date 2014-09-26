#include "child.h"
#include "common.h"
#include "cap.h"
#include "devpts.h"
#include "opts.h"
#include "home.h"
#include "mounts.h"
#include "network.h"
#include "path.h"
#include "run.h"
#include "tty.h"
#include "x11.h"
#include <unistd.h>
#include <sys/mount.h>

int child_main(void *arg) {
  appjail_options *opts = (appjail_options*)arg;

  drop_caps();
  init_libmount();
  /* Set up the private network */
  if(opts->unshare_network)
    if( configure_loopback_interface() != 0 )
      fprintf(stderr, "Unable to configure loopback interface\n");
  /* Make our mount a slave of the host - this will make sure our
   * mounts do not propagate to the host. If we made everything
   * private now, we would lose the ability to keep anything as slave.
   */
  set_mount_propagation_slave();
  /* Mount tmpfs to contain our private data to APPJAIL_SWAPDIR */
  if( cap_mount("appjail", APPJAIL_SWAPDIR, "tmpfs", MS_NODEV | MS_NOSUID, "") == -1 )
    errExit("mount -t tmpfs appjail " APPJAIL_SWAPDIR);
  /* Change into the temporary directory */
  if(chdir(APPJAIL_SWAPDIR) == -1)
    errExit("chdir()");

  /* Bind directories and files that may disappear */
  get_home_directory(opts->homedir);
  get_tty(opts);
  if(opts->keep_x11)
    /* Get X11 socket directory and xauth data */
    get_x11(opts);

  /* clean up the mounts, making almost everything private */
  sanitize_mounts(opts);

  /* set up our private mounts */
  setup_path("tmp", "/tmp", 01777);
  setup_path("vartmp", "/var/tmp", 01777);
  setup_path("home", "/home", 0755);
  if(!opts->keep_shm)
    setup_path("shm", "/dev/shm", 01777);
  setup_devpts();

  /* set up the tty */
  setup_tty(opts);
  /* set up /run */
  setup_run(opts);
  /* set up home directory using the one we bound earlier
   * WARNING: We change the current directory from APPJAIL_SWAPDIR to the home directory */
  setup_home_directory(opts->user);
  if(opts->keep_x11)
    /* Set up X11 socket directory and xauth data */
    setup_x11();

  /* unmount our temporary directory */
  if( cap_umount2(APPJAIL_SWAPDIR, 0) == -1 )
    errExit("umount " APPJAIL_SWAPDIR);

  /* Make some permissions consistent */
  cap_chown("/tmp", 0, 0);
  cap_chown("/var/tmp", 0, 0);
  cap_chown("/home", 0, 0);
  if(!opts->keep_shm)
    cap_chown("/dev/shm", 0, 0);

  /* We drop all capabilities from the permitted capability set */
  drop_caps_forever();

  if(opts->argv[0] != NULL) {
    execvp(opts->argv[0], opts->argv);
    errExit("execvp");
  }
  else {
    execl("/bin/sh", "/bin/sh", "-i", NULL);
    errExit("execl");
  }
}
