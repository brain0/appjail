#include "common.h"
#include "opts.h"
#include <getopt.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>

#define NUM_ENTRIES 10

static void version() {
  printf("appjail " APPJAIL_VERSION " by Thomas Bächler <thomas@archlinux.org>\n"
         "\n"
         "Available at https://github.com/brain0/appjail\n"
         "Licensed under the conditions of the WTFPL (http://www.wtfpl.net/)\n"
         "\n");
}

static void usage() {
  printf("Usage: appjail [OPTIONS] [COMMAND]\n"
         "\n"
         "Start COMMAND in an isolated environment. If COMMAND is not given, it is set to '/bin/sh -i'.\n"
         "\n"
         "Options:\n"
         "  -h, --help              Print command help and exit.\n"
         "  -d, --daemonize         Run the jailed process in the background.\n"
         "  -i, --initstub          Run a stub init process inside the jail.\n"
         "  -p, --allow-new-privs   Don't prevent setuid binaries from raising privileges.\n"
         "  --keep-shm              Keep the host's /dev/shm directory.\n"
         "  --keep-ipc-namespace    Stay in the host's IPC namespace. This is necessary for\n"
         "                          Xorg's MIT-SHM extension.\n"
         "  -H, --homedir <DIR>     Use DIR as home directory instead of a temporary one.\n"
         "  -K, --keep <DIR>        Do not Unmount DIR inside the jail.\n"
         "                          This option also affects all mounts that are parents of DIR.\n"
         "  --keep-full <DIR>       Like --keep, but also affects all submounts of DIR.\n"
         "  -S, --shared <DIR>      Do not force private mount propagation on submounts of DIR.\n"
         "  -M, --mask <DIR>        Make DIR and all its subdirectories inaccessible in the jail.\n"
         "  -X, --x11               Allow X11 access.\n"
         "  --x11-trusted           Generate a trusted X11 cookie (an untrusted cookie is used by default).\n"
         "  --x11-timeout <N>       If no X11 client is connected for N seconds, the cookie is revoked.\n"
         "  -N, --private-network   Isolate from the host network.\n"
         "  -R, --run <MODE>        Determine how to handle the /run directory.\n"
         "                           host:    Keep the host's /run directory\n"
         "                           user:    Only keep /run/user/UID\n"
         "                           private: Use a private /run directory\n"
         "  --(no-)run-media        Determine whether /run/media/USER will be available in the jail.\n"
         "                          This option has no effect with --run=host.\n"
         "  --system-bus            Determine whether the host's DBUS socket directory (/run/dbus)\n"
         "                          should be available in the jail.\n"
         "                          This option has no effect with --run=host.\n"
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

#define OPT_KEEP_SHM 256
#define OPT_KEEP_FULL 257
#define OPT_RUN_MEDIA 258
#define OPT_NO_RUN_MEDIA 259
#define OPT_KEEP_IPC_NAMESPACE 260
#define OPT_X11_TRUSTED 261
#define OPT_X11_TIMEOUT 262
#define OPT_KEEP_SYSTEM_BUS 263

appjail_options *parse_options(int argc, char *argv[], const appjail_config *config) {
  int opt;
  appjail_options *opts;
  struct passwd *pw;
  static struct option long_options[] = {
    { "version",            no_argument,       0,  'V'                    },
    { "help",               no_argument,       0,  'h'                    },
    { "allow-new-privs",    no_argument,       0,  'p'                    },
    { "homedir",            required_argument, 0,  'H'                    },
    { "keep-shm",           no_argument,       0,  OPT_KEEP_SHM           },
    { "keep-ipc-namespace", no_argument,       0,  OPT_KEEP_IPC_NAMESPACE },
    { "keep",               required_argument, 0,  'K'                    },
    { "keep-full",          required_argument, 0,  OPT_KEEP_FULL          },
    { "shared",             required_argument, 0,  'S'                    },
    { "x11",                no_argument,       0,  'X'                    },
    { "x11-trusted",        no_argument,       0,  OPT_X11_TRUSTED        },
    { "x11-timeout",        required_argument, 0,  OPT_X11_TIMEOUT        },
    { "private-network",    no_argument,       0,  'N'                    },
    { "no-private-network", no_argument,       0,  'n'                    },
    { "run",                required_argument, 0,  'R'                    },
    { "run-media",          no_argument,       0,  OPT_RUN_MEDIA          },
    { "no-run-media",       no_argument,       0,  OPT_NO_RUN_MEDIA       },
    { "system-bus",         no_argument,       0,  OPT_KEEP_SYSTEM_BUS    },
    { "mask",               required_argument, 0,  'M'                    },
    { "daemonize",          no_argument,       0,  'd'                    },
    { "initstub",           no_argument,       0,  'i'                    },
    { 0,                    0,                 0,  0                      }
  };

  if((opts = malloc(sizeof(appjail_options))) == NULL)
    errExit("malloc");

  /* special options */
  opts->uid = getuid();
  errno = 0;
  if((pw = getpwuid(opts->uid)) == NULL)
    errExit("getpwuid");
  opts->user = strdup(pw->pw_name);
  /* defaults */
  opts->allow_new_privs = false;
  opts->keep_shm = false;
  opts->keep_ipc_namespace = false;
  opts->homedir = NULL;
  opts->keep_x11 = false;
  opts->x11_trusted = false;
  opts->x11_timeout = 60;
  opts->unshare_network = config->default_private_network;
  opts->run_mode = config->default_run_mode;
  opts->bind_run_media = config->default_bind_run_media;
  opts->keep_system_bus = false;
  opts->daemonize = false;
  opts->initstub = false;
  /* initialize directory lists */
  opts->keep_mounts = strlist_new();
  opts->keep_mounts_full = strlist_new();
  opts->shared_mounts = strlist_new();
  opts->mask_directories = strlist_new();

  while((opt = getopt_long(argc, argv, "+:hVpH:K:S:XNnR:M:di", long_options, NULL)) != -1) {
    switch(opt) {
      case 'V':
        version();
        exit(EXIT_SUCCESS);
        break;
      case 'h':
        version();
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
      case OPT_KEEP_SHM:
        opts->keep_shm = true;
        break;
      case OPT_KEEP_IPC_NAMESPACE:
        opts->keep_ipc_namespace = true;
        break;
      case 'K':
        strlist_append(opts->keep_mounts, remove_trailing_slash(optarg));
        break;
      case OPT_KEEP_FULL:
        strlist_append(opts->keep_mounts_full, remove_trailing_slash(optarg));
        break;
      case 'S':
        strlist_append(opts->shared_mounts, remove_trailing_slash(optarg));
        break;
      case 'X':
        opts->keep_x11 = true;
        break;
      case OPT_X11_TRUSTED:
        opts->x11_trusted = true;
        break;
      case OPT_X11_TIMEOUT:
        if(!string_to_unsigned_integer(&(opts->x11_timeout), optarg))
          errExitNoErrno("Invalid argument to --x11-timeout.");
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
      case OPT_RUN_MEDIA:
        opts->bind_run_media = true;
        break;
      case OPT_NO_RUN_MEDIA:
        opts->bind_run_media = false;
        break;
      case OPT_KEEP_SYSTEM_BUS:
        opts->keep_system_bus = true;
        break;
      case 'M':
        strlist_append(opts->mask_directories, remove_trailing_slash(optarg));
        break;
      case 'd':
        opts->daemonize = true;
        break;
      case 'i':
        opts->initstub = true;
        break;
      case ':':
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
        exit(EXIT_FAILURE);
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

  opts->special_mounts = strlist_new();
  strlist_append_copy(opts->special_mounts, "/dev");
  strlist_append_copy(opts->special_mounts, "/proc");
  strlist_append_copy(opts->special_mounts, "/run");
  strlist_append_copy(opts->special_mounts, APPJAIL_SWAPDIR);

  return opts;
}

void free_options(appjail_options *opts) {
  strlist_free(opts->keep_mounts);
  strlist_free(opts->keep_mounts_full);
  strlist_free(opts->shared_mounts);
  strlist_free(opts->special_mounts);
  strlist_free(opts->mask_directories);
  free(opts->user);
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
