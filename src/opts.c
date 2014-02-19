#include "common.h"
#include "opts.h"
#include <getopt.h>
#include <string.h>

#define NUM_ENTRIES 10

static void usage() {
  printf("Usage: appjail [OPTIONS] [COMMAND]\n"
         "\n"
         "Start COMMAND in an isolated environment. If COMMAND is not given, it is set to '/bin/sh -i'.\n"
         "\n"
         "Options:\n"
         "  -h, --help              Print command help and exit.\n"
         "  -p, --allow-new-privs   Don't prevent setuid binaries from raising privileges.\n"
         "  --keep-shm              Keep the host's /dev/shm directory.\n"
         "  -H, --homedir <DIR>     Use DIR as home directory instead of a temporary one.\n"
         "\n");
}

char *remove_trailing_slash(const char *p) {
  char *r = strdup(p);
  size_t len = strlen(r);

  while(len > 0 && r[len-1] == '/') {
    r[--len] = '\0';
  }
  return r;
}

static void add_array_entry(char ***array, unsigned int *size, unsigned int *num, char *entry) {
  if(*num >= *size)
    if((*array = realloc(*array, (*size*=2)*sizeof(char*))) == NULL)
      errExit("realloc");
  if((*array)[*num-1] != NULL)
    errExitNoErrno("Internal error");
  (*array)[*num-1] = entry;
  (*array)[(*num)++] = NULL;
}

#define ADD_ARRAY_ENTRY_UNMOUNT(p) do {\
    char *t = remove_trailing_slash(p);\
    size_t len = strlen(t), lenp = strlen(APPJAIL_SWAPDIR);\
    if(!strncmp(APPJAIL_SWAPDIR, t, len) && (lenp == len || (lenp > len && APPJAIL_SWAPDIR[len] == '/')))\
      errExitNoErrno("Unmount rules would unmount " APPJAIL_SWAPDIR ", aborting.");\
    add_array_entry(&(opts->unmount_directories), &unmount_directories_size, &unmount_directories_num, t);\
  } while(0)
#define ADD_ARRAY_ENTRY_SHARED(p) add_array_entry(&(opts->shared_directories), &shared_directories_size, &shared_directories_num, remove_trailing_slash(p))

appjail_options *parse_options(int argc, char *argv[]) {
  int opt;
  unsigned int unmount_directories_num,
      unmount_directories_size,
      shared_directories_num,
      shared_directories_size;
  appjail_options *opts;
  static struct option long_options[] = {
    { "help",            no_argument,       0,  'h'  },
    { "allow-new-privs", no_argument,       0,  'p'  },
    { "homedir",         required_argument, 0,  'H'  },
    { "keep-shm",        no_argument,       0,  256  },
    { 0,                 0,                 0,  0    }
  };

  if((opts = malloc(sizeof(appjail_options))) == NULL)
    errExit("malloc");

  /* defaults */
  opts->allow_new_privs = false;
  opts->keep_shm = false;
  opts->homedir = NULL;
  /* initialize directory lists */
  opts->unmount_directories = malloc(NUM_ENTRIES*sizeof(char*));
  unmount_directories_size = NUM_ENTRIES;
  unmount_directories_num = 1;
  opts->unmount_directories[0] = NULL;
  opts->shared_directories = malloc(NUM_ENTRIES*sizeof(char*));
  shared_directories_size = NUM_ENTRIES;
  shared_directories_num = 1;
  opts->shared_directories[0] = NULL;

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
      case 256:
        opts->keep_shm = true;
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

  if(opts->keep_shm)
    ADD_ARRAY_ENTRY_SHARED("/dev/shm");
  else
    ADD_ARRAY_ENTRY_UNMOUNT("/dev/shm");
  ADD_ARRAY_ENTRY_UNMOUNT("/dev/pts");
  ADD_ARRAY_ENTRY_UNMOUNT("/tmp");
  ADD_ARRAY_ENTRY_UNMOUNT("/var/tmp");
  ADD_ARRAY_ENTRY_UNMOUNT("/home");

  return opts;
}

void free_options(appjail_options *opts) {
  char** p;
  for(p = opts->unmount_directories; *p!=NULL; ++p)
    free(*p);
  for(p = opts->shared_directories; *p!=NULL; ++p)
    free(*p);
  free(opts->unmount_directories);
  free(opts->shared_directories);
  free(opts);
}
