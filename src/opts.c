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
         "  -K, --keep <DIR>        Do not Unmount DIR inside the jail.\n"
         "                          This option also affects all mounts that are parents of DIR.\n"
         "  --keep-full <DIR>       Like --keep, but also affects all submounts of DIR.\n"
         "  -S, --shared <DIR>      Do not force private mount propagation on submounts of DIR.\n"
         "  -X, --x11               Allow X11 access.\n"
         "  -N, --private-network   Isolate from the host network.\n"
         "  -R, --run <MODE>        Determine how to handle the /run directory.\n"
         "                           host:    Keep the host's /run directory\n"
         "                           user:    Only keep /run/user/UID\n"
         "                           private: Use a private /run directory\n"
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

#define ADD_ARRAY_ENTRY_KEEP(p) add_array_entry(&(opts->keep_mounts), &keep_mounts_size, &keep_mounts_num, remove_trailing_slash(p))
#define ADD_ARRAY_ENTRY_KEEP_FULL(p) add_array_entry(&(opts->keep_mounts_full), &keep_mounts_full_size, &keep_mounts_full_num, remove_trailing_slash(p))
#define ADD_ARRAY_ENTRY_SHARED(p) add_array_entry(&(opts->shared_mounts), &shared_mounts_size, &shared_mounts_num, remove_trailing_slash(p))

appjail_options *parse_options(int argc, char *argv[], appjail_config *config) {
  int opt;
  unsigned int keep_mounts_num,
      keep_mounts_size,
      keep_mounts_full_num,
      keep_mounts_full_size,
      shared_mounts_num,
      shared_mounts_size;
  appjail_options *opts;
  static struct option long_options[] = {
    { "help",               no_argument,       0,  'h'  },
    { "allow-new-privs",    no_argument,       0,  'p'  },
    { "homedir",            required_argument, 0,  'H'  },
    { "keep-shm",           no_argument,       0,  256  },
    { "keep",               required_argument, 0,  'K'  },
    { "keep-full",          required_argument, 0,  257  },
    { "shared",             required_argument, 0,  'S'  },
    { "x11",                no_argument,       0,  'X'  },
    { "private-network",    no_argument,       0,  'N'  },
    { "no-private-network", no_argument,       0,  'n'  },
    { "run",                required_argument, 0,  'R'  },
    { 0,                    0,                 0,  0    }
  };

  if((opts = malloc(sizeof(appjail_options))) == NULL)
    errExit("malloc");

  /* defaults */
  opts->allow_new_privs = false;
  opts->keep_shm = false;
  opts->homedir = NULL;
  opts->keep_x11 = false;
  opts->unshare_network = config->default_private_network;
  opts->run_mode = RUN_PRIVATE;
  /* initialize directory lists */
  opts->keep_mounts = malloc(NUM_ENTRIES*sizeof(char*));
  keep_mounts_size = NUM_ENTRIES;
  keep_mounts_num = 1;
  opts->keep_mounts[0] = NULL;
  opts->keep_mounts_full = malloc(NUM_ENTRIES*sizeof(char*));
  keep_mounts_full_size = NUM_ENTRIES;
  keep_mounts_full_num = 1;
  opts->keep_mounts_full[0] = NULL;
  opts->shared_mounts = malloc(NUM_ENTRIES*sizeof(char*));
  shared_mounts_size = NUM_ENTRIES;
  shared_mounts_num = 1;
  opts->shared_mounts[0] = NULL;

  while((opt = getopt_long(argc, argv, "+:hpH:K:S:XNnR:", long_options, NULL)) != -1) {
    switch(opt) {
      case 'h':
        usage();
        exit(EXIT_SUCCESS);
        break;
      case 'p':
        if(!config->allow_new_privs_permitted)
          errExitNoErrno("Using the --allow-new-privs option is not permitted.");
        opts->allow_new_privs = true;
        break;
      case 'H':
        opts->homedir = optarg;
        break;
      case 256:
        opts->keep_shm = true;
        break;
      case 'K':
        ADD_ARRAY_ENTRY_KEEP(optarg);
        break;
      case 257:
        ADD_ARRAY_ENTRY_KEEP_FULL(optarg);
        break;
      case 'S':
        ADD_ARRAY_ENTRY_SHARED(optarg);
        break;
      case 'X':
        opts->keep_x11 = true;
        break;
      case 'N':
        opts->unshare_network = true;
        break;
      case 'n':
        opts->unshare_network = false;
        break;
      case 'R':
        if(!string_to_run_mode(&(opts->run_mode), optarg))
          errExitNoErrno("Invalid argument to -R/--run.");
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

  opts->special_mounts = malloc(5*sizeof(char*));
  opts->special_mounts[0] = "/dev";
  opts->special_mounts[1] = "/proc";
  opts->special_mounts[2] = "/run";
  opts->special_mounts[3] = APPJAIL_SWAPDIR;
  opts->special_mounts[4] = NULL;

  return opts;
}

void free_options(appjail_options *opts) {
  char** p;
  for(p = opts->keep_mounts; *p!=NULL; ++p)
    free(*p);
  for(p = opts->keep_mounts_full; *p!=NULL; ++p)
    free(*p);
  for(p = opts->shared_mounts; *p!=NULL; ++p)
    free(*p);
  free(opts->keep_mounts);
  free(opts->keep_mounts_full);
  free(opts->shared_mounts);
  free(opts->special_mounts);
  free(opts);
}

bool string_to_run_mode(run_mode_t *result, const char *s) {
  bool ret = true;

  if(!strcmp(s, "host"))
    *result = RUN_HOST;
  else if(!strcmp(s, "user"))
    *result = RUN_USER;
  else if(!strcmp(s, "private"))
    *result = RUN_PRIVATE;
  else
    ret = false;

  return ret;
}
