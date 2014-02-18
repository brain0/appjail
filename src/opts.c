#include "common.h"
#include "opts.h"
#include <getopt.h>

static void usage() {
  printf("Usage: appjail [OPTIONS] [COMMAND]\n"
         "\n"
         "Start COMMAND in an isolated environment. If COMMAND is not given, it is set to '/bin/sh -i'.\n"
         "\n"
         "Options:\n"
         "  -h, --help              Print command help and exit.\n"
         "  -p, --allow-new-privs   Don't prevent setuid binaries from raising privileges.\n"
         "  -H, --homedir <DIR>     Use DIR as home directory instead of a temporary one.\n"
         "\n");
}

appjail_options *parse_options(int argc, char *argv[]) {
  int opt;
  appjail_options *opts;
  static struct option long_options[] = {
    { "help",            no_argument,       0,  'h' },
    { "allow-new-privs", no_argument,       0,  'p' },
    { "homedir",         required_argument, 0,  'H' },
    { 0,                 0,                 0,  0   }
  };

  if((opts = malloc(sizeof(appjail_options))) == NULL)
    errExit("malloc");

  /* defaults */
  opts->allow_new_privs = false;
  opts->homedir = NULL;
  /* */
  opts->special_directories.unmount_directories = malloc(6*sizeof(char*));
  opts->special_directories.unmount_directories[0] = "/dev/shm";
  opts->special_directories.unmount_directories[1] = "/dev/pts";
  opts->special_directories.unmount_directories[2] = "/tmp";
  opts->special_directories.unmount_directories[3] = "/var/tmp";
  opts->special_directories.unmount_directories[4] = "/home";
  opts->special_directories.unmount_directories[5] = NULL;
  opts->special_directories.shared_directories = malloc(sizeof(char*));
  opts->special_directories.shared_directories[0] = NULL;

  while((opt = getopt_long(argc, argv, "+:hpH:", long_options, NULL)) != -1) {
    switch(opt) {
      case 'h':
        usage();
        exit(EXIT_SUCCESS);
        break;
      case 'p':
        opts->allow_new_privs = true;
        break;
      case 'H':
        opts->homedir = optarg;
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

  return opts;
}

void free_options(appjail_options *opts) {
  /* unmount directories and shared_directories only contain
   * pointers to string constants and to parts of the
   * argument list, we cannot free any of those */
  free(opts->special_directories.unmount_directories);
  free(opts->special_directories.shared_directories);
  free(opts);
}
