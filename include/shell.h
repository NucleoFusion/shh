#ifndef SHELL_H
#define SHELL_H

char *shh_readline();
char **shh_splitline();
int shh_execute(char **args);
int shh_run(char **args);

void shh_loop();

#endif
