#define _POSIX_C_SOURCE 200809
#include "../include/shh_shell.h"
#include "builtins.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHH_RL_BUFSIZE 1024
#define SHH_TOK_BUFSIZE 64
#define SHH_MAX_PIPES 64
#define SHH_TOK_DELIMITER " \t\r\n\a"
#define SHH_PIPE_DELIMITER "|"

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
  char **tokens = malloc(sizeof(char *) * bufsize);
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
      tokens = realloc(tokens, sizeof(char *) * bufsize);
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

Pipeline *shh_parse_pipline(char *line) {
  Pipeline *pipe = malloc(sizeof(Pipeline));
  pipe->commands = malloc(sizeof(Command) * SHH_MAX_PIPES);
  pipe->count = 0;

  char *saveptr;
  char *cmd = strtok_r(line, SHH_PIPE_DELIMITER, &saveptr);
  while (cmd != NULL) {
    char **args = shh_splitline(cmd);

    // Init
    pipe->commands[pipe->count].redirect_in = NULL;
    pipe->commands[pipe->count].redirect_out = NULL;
    pipe->commands[pipe->count].redirect_err = NULL;
    pipe->commands[pipe->count].append = 0;
    pipe->commands[pipe->count].args = args;
    pipe->commands[pipe->count].append = 0;

    for (int i = 0; args[i] != NULL; i++) {
      if (strcmp(">", args[i]) == 0) {
        pipe->commands[pipe->count].redirect_out = args[i + 1];
        args[i] = NULL;
        break;
      } else if (strcmp(">>", args[i]) == 0) {
        pipe->commands[pipe->count].redirect_out = args[i + 1];
        pipe->commands[pipe->count].append = 1;
        args[i] = NULL;
        break;
      } else if (strcmp("2>", args[i]) == 0) {
        pipe->commands[pipe->count].redirect_err = args[i + 1];
        pipe->commands[pipe->count].append = 1;
        args[i] = NULL;
        break;
      } else if (strcmp("<", args[i]) == 0) {
        pipe->commands[pipe->count].redirect_in = args[i + 1];
        pipe->commands[pipe->count].append = 1;
        args[i] = NULL;
        break;
      }
    }
    pipe->count++;

    cmd = strtok_r(NULL, SHH_PIPE_DELIMITER, &saveptr);
  }

  return pipe;
}

int shh_exec_child(char **args) {
  if (execvp(args[0], args) == -1) {
    perror("shh");
  }

  exit(EXIT_FAILURE);
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

int shh_execute_pipeline(Pipeline *p) {
  int pipes[p->count - 1][2];
  for (int i = 0; i < p->count - 1; i++) {
    pipe(pipes[i]);
  }

  for (int i = 0; i < p->count; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      if (i > 0) { // If not first pipe
        dup2(pipes[i - 1][0], STDIN_FILENO);
      }

      if (i < p->count - 1) {
        dup2(pipes[i][1], STDOUT_FILENO);
      }

      for (int j = 0; j < p->count - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      // Handling Redirects
      if (p->commands[i].redirect_out != NULL) {
        int flags =
            O_WRONLY | O_CREAT | (p->commands[i].append ? O_APPEND : O_TRUNC);
        int fd = open(p->commands[i].redirect_out, flags, 0644);
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }
      if (p->commands[i].redirect_err != NULL) {
        int flags = O_WRONLY | O_CREAT | O_TRUNC;
        int fd = open(p->commands[i].redirect_err, flags, 0644);
        dup2(fd, STDERR_FILENO);
        close(fd);
      }
      if (p->commands[i].redirect_in != NULL) {
        int flags = O_RDONLY;
        int fd = open(p->commands[i].redirect_in, flags, 0644);
        dup2(fd, STDIN_FILENO);
        close(fd);
      }

      if (execvp(p->commands[i].args[0], p->commands[i].args) == -1) {
        perror("shh");
      }
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < p->count - 1; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }
  for (int i = 0; i < p->count; i++) {
    wait(NULL);
  }
}

void print_pipeline(Pipeline *p) {
  printf("Pipeline (%d commands):\n", p->count);
  for (int i = 0; i < p->count; i++) {
    printf("  [%d]: ", i);
    char **args = p->commands[i].args;
    for (int j = 0; args[j] != NULL; j++) {
      printf("'%s' ", args[j]);
    }
    printf("\n");
  }
}

void shh_loop() {
  char *line;
  Pipeline *pipe;
  int status;

  do {
    printf("# ");
    line = shh_readline();
    pipe = shh_parse_pipline(line);
    // print_pipeline(pipe);
    status = shh_execute_pipeline(pipe);
  } while (status);
}
