#include "../include/shh_shell.h"
#include <signal.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  signal(SIGINT, SIG_IGN);
  shh_loop();

  return EXIT_SUCCESS;
}
