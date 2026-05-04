#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *builtin_str[3] = {"cd", "help", "exit"};

int shh_num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

int shh_help(char **args) {
  printf("SHH - The Shit Shell\n");
  printf("The following are builtin:\n");

  for (int i = 0; i < shh_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  printf("use the power of \"man\" for info on other programs \n");
  return 1;
}

int shh_cd(char **args) {
  if (args[1] == NULL) {
    char *home = getenv("HOME");
    if (home == NULL) {
      fprintf(stderr, "shh: HOME not set, expected argument to \"cd\" \n");
    }
    if (chdir(home) != 0) {
      perror("shh");
    }
  } else {
    if (args[1][0] == '~') {
      char *home = getenv("HOME");
      if (home == NULL) {
        fprintf(stderr, "shh: HOME not set, expected argument to \"cd\" \n");
      }
      char *result = malloc(strlen(home) + strlen(args[1]));
      sprintf(result, "%s%s", home, args[1] + 1);
      args[1] = result;
    }

    if (chdir(args[1]) != 0) {
      perror("shh");
    }
  }

  return 1;
}

int shh_exit(char **args) { return 0; }

int (*builtin_func[3])(char **) = {&shh_cd, &shh_help, &shh_exit};
