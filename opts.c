#include "common.h"
#include "opts.h"
#include <getopt.h>

static void usage() {
}

void parse_options(appjail_options *opts, int argc, char *argv[]) {
  int opt;
  static struct option long_options[] = {
    { "help",            no_argument,       0,  'h' },
    { "allow-new-privs", no_argument,       0,  'p' },
    { 0,                 0,                 0,  0   }
  };

  /* defaults */
  opts->allow_new_privs = false;

  while((opt = getopt_long(argc, argv, "+:hp", long_options, NULL)) != -1) {
    switch(opt) {
      case 'h':
        usage();
        exit(EXIT_SUCCESS);
        break;
      case 'p':
        opts->allow_new_privs = true;
        break;
      case ':':
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
        exit(EXIT_FAILURE);;
      case '?':
        if(optopt == '\0')
          fprintf(stderr, "Invalid option: %s\n", argv[optind-1]);
        else
          fprintf(stderr, "Invalid option: -%c\n", optopt);
        exit(EXIT_FAILURE);
      default:
        errExitNoErrno("Unknown error while parsing options.");
        exit(EXIT_FAILURE);
    }
  }
  opts->argv = &(argv[optind]);
}
