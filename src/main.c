#include "appjail.h"
#include "cap.h"
#include "initstub.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if(!strcmp(argv[0], "initstub")) {
    /* appjail is rexecuting itself as stub init process
     *
     * First, ensure we have no privileges (we may have some
     * when appjail was started with the -p option).
     */
    drop_caps_forever();
    /* If we are not PID 1, we abort right away. */
    if(getpid() == 1)
      return initstub_main(argc, argv);
    return EXIT_FAILURE;
  }

  /* run appjail */
  return appjail_main(argc, argv);
}
