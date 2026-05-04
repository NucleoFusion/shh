#ifndef SHELL_H
#define SHELL_H

typedef struct {
  char **args;
  char *redirect_in;
  char *redirect_out;
  char *redirect_err;
  int append;
} Command;

typedef struct {
  Command *commands;
  int count;
} Pipeline;

Pipeline *shh_parse_pipline(char *line);
int shh_execute_pipeline(Pipeline *pipe);

char *shh_readline();
char **shh_splitline(char *line);

void shh_loop();

#endif
