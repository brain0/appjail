#include "initstub.h"
#include "command.h"
#include "common.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void run_initstub(char *argv[], char **envp) {
  char **new_argv;
  int argc = 0, i;

  while(argv[argc] != NULL)
    argc++;

  if(argc > 0) {
    new_argv = malloc((argc+2)*sizeof(char*));
    new_argv[0] = "initstub";
    for(i = 0; i <= argc; i++)
      new_argv[i + 1] = argv[i];
  }
  else {
    new_argv = malloc(4*sizeof(char*));
    new_argv[0] = "initstub";
    new_argv[1] = "/bin/sh";
    new_argv[2] = "-i";
    new_argv[3] = NULL;
  }

  if(envp == NULL)
    execv("/proc/self/exe", new_argv);
  else
    execve("/proc/self/exe", new_argv, envp);
  errExit("Failed to execute init stub.");
}

int initstub_main(int argc, char *argv[]) {
  int status, i;
  char *argv0 = argv[0];

  if(argc <= 1)
    return EXIT_FAILURE;

  run_command(argv[1], argv+1, false);

  for(i = 0; i < argc; ++i)
    memset(argv[i], 0, strlen(argv[i]));
  strcpy(argv0, "initstub");

  while(waitpid(-1, &status, 0) != -1);
  if(errno == ECHILD) {
    fprintf(stderr, "All child processes exited. Exiting.\n");
    return EXIT_SUCCESS;
  }
  else {
    fprintf(stderr, "An error occurred during waitpid(): %s\n", strerror(errno));
    return EXIT_FAILURE;
  }
}
