#include "common.h"
#include "opts.h"
#include <getopt.h>

static void usage() {
}

void parse_options(appjail_options *opts, int argc, char *argv[]) {
  int opt;
  static struct option long_options[] = {
    { "help",            no_argument,       0,  'h' },
    { 0,                 0,                 0,  0   }
  };

  /* defaults */

  while((opt = getopt_long(argc, argv, "+:h", long_options, NULL)) != -1) {
    switch(opt) {
      case 'h':
        usage();
        exit(EXIT_SUCCESS);
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
