/*

Gordon compiles with this on his powerbook:

  cc -o demoIO demoIO.c \
    -I/Users/gk/teem/include -I/sw/include \
    -L/Users/gk/teem/darwin.32/lib -L/sw/lib \
    -lteem -lpng -lbz2 -lz -lm

*/

#include <teem/nrrd.h>

void
demoIO(char *filename) {
  char me[]="demoIO", newname[]="foo.nrrd", *err;
  Nrrd *nin;

  /* create a nrrd; at this point this is just an empty container */
  nin = nrrdNew();

  /* read in the nrrd from file */
  if (nrrdLoad(nin, filename, NULL)) {
    err = biffGetDone(NRRD);
    fprintf(stderr, "%s: trouble reading \"%s\":\n%s", me, filename, err);
    free(err);
    return;
  }

  /* say something about the array */
  printf("%s: \"%s\" is a %d-dimensional nrrd of type %d (%s)\n", 
         me, filename, nin->dim, nin->type,
         airEnumStr(nrrdType, nin->type));
  printf("%s: the array contains %d elements, each %d bytes in size\n",
         me, (int)nrrdElementNumber(nin), (int)nrrdElementSize(nin));

  /* write out the nrrd to a different file */
  if (nrrdSave(newname, nin, NULL)) {
    err = biffGetDone(NRRD);
    fprintf(stderr, "%s: trouble writing \"%s\":\n%s", me, newname, err);
    free(err);
    return;
  }

  /* blow away both the Nrrd struct *and* the memory at nin->data
     (nrrdNix() frees the struct but not the data,
     nrrdEmpty() frees the data but not the struct) */
  nrrdNuke(nin);

  return;
}

int
main(int argc, char **argv) {

  if (2 != argc) {
    fprintf(stderr, "usage: demoIO <filename>\n");
    return 1;
  }

  demoIO(argv[1]);

  return 0;
}
