#include <stdio.h>

int
main(int argc, char *argv[]) {
  int i;

  for (i=1; i<=argc-1; i++) {
    printf("%3d: %s\n", i-1, argv[i]);
  }
  exit(0);
}
