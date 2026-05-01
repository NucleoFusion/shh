#ifndef BUILTINS_H
#define BUILTINS_H

int shh_cd(char **args);
int shh_help(char **args);
int shh_exit(char **args);

extern char *builtin_str[3];
extern int (*builtin_func[3])(char **);

int shh_num_builtins();

#endif
