#include "shell.h"
#include "builtins.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHH_RL_BUFSIZE 1024
#define SHH_TOK_BUFSIZE 64
#define SHH_TOK_DELIMITER " \t\r\n\a"

char *shh_readline() {
  int bufsize = SHH_RL_BUFSIZE;
  int position = 0;
  char *buf = malloc(sizeof(char) * bufsize);
  int c;

  if (!buf) {
    fprintf(stderr, "shh: memory allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();

    if (c == EOF || c == '\n') {
      buf[position] = '\0';
      return buf;
    } else {
      buf[position] = c;
    }
    position++;

    if (position >= bufsize) {
      bufsize += SHH_RL_BUFSIZE;
      buf = realloc(buf, bufsize);
      if (!buf) {
        fprintf(stderr, "shh: memory allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

char **shh_splitline(char *line) {
  int bufsize = SHH_TOK_BUFSIZE;
  int position = 0;
  char **tokens = malloc(sizeof(char) * bufsize);
  char *token;

  if (!tokens) {
    fprintf(stderr, "shh: memory allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, SHH_TOK_DELIMITER);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += SHH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize);
      if (!tokens) {
        fprintf(stderr, "shh: memory allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, SHH_TOK_DELIMITER);
  }

  tokens[position] = NULL;
  return tokens;
}

int shh_run(char **args) {
  pid_t pid, wpid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      perror("shh");
    }

    exit(EXIT_FAILURE);
  } else if (pid < 0) { // ERROR forking
    perror("shh");
  } else {
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int shh_execute(char **args) {
  if (args[0] == NULL) {
    return 1;
  }

  for (int i = 0; i < shh_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return shh_run(args);
}

void shh_loop() {
  char *line;
  char **args;
  int status;

  do {
    printf("# ");
    line = shh_readline();
    args = shh_splitline(line);
    status = shh_execute(args);
  } while (status);
}
