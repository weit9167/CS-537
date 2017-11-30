#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include "sort.h"
#include <sys/types.h>
#include <sys/stat.h>

int column = 0;

int
compare(const void *a, const void *b) {
  int actualColA = 0;
  if (column > ((*((rec_dataptr_t*)a)).data_ints-1)) {
    actualColA = ((*((rec_dataptr_t*)a)).data_ints-1);
  } else {
    actualColA = column;
  }

  int actualColB = 0;
  if (column > ((*((rec_dataptr_t*)b)).data_ints-1)) {
    actualColB = ((*((rec_dataptr_t*)b)).data_ints-1);
  } else {
    actualColB = column;
  }

  if (((*((rec_dataptr_t*)a)).data_ptr[actualColA])
  < ((*((rec_dataptr_t*)b)).data_ptr[actualColB]))
    return -1;
  else if (((*((rec_dataptr_t*)a)).data_ptr[actualColA])
  > ((*((rec_dataptr_t*)b)).data_ptr[actualColB]))
    return 1;

  return 0;
}


int
main(int argc, char *argv[]) {
  // arguments
  char *inFile = "/no/such/file";
  char *outFile = "/no/such/file";
  int c;
  opterr = 0;

  c = getopt(argc, argv, "i:");
  if (c == -1) {
    fprintf(stderr,
    "Usage: ./varsort -i inputfile -o outputfile [-c column]\n");
    exit(1);
  }
  if (c != 'i') {
    fprintf(stderr,
    "Usage: ./varsort -i inputfile -o outputfile [-c column]\n");
    exit(1);
  } else {
    inFile = optarg;
  }

  c = getopt(argc, argv, "o:");
  if (c == -1) {
    fprintf(stderr,
    "Usage: ./varsort -i inputfile -o outputfile [-c column]\n");
    exit(1);
  }
  if (c != 'o') {
    fprintf(stderr,
    "Usage: ./varsort -i inputfile -o outputfile [-c column]\n");
    exit(1);
  } else {
    outFile = optarg;
  }

  c = getopt(argc, argv, "c:");
  if (c != -1) {
    if (c == 'c') {
      column = atoi(optarg);
      if (column < 0) {
        fprintf(stderr,
        "Error: Column should be a non-negative integer\n");
        exit(1);
      }
    } else {
      fprintf(stderr,
      "Usage: ./varsort -i inputfile -o outputfile [-c column]\n");
      exit(1);
    }
  }

  int infd = open(inFile, O_RDONLY);
  if (infd < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", inFile);
    exit(1);
  }

  int outfd = open(outFile, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if (outfd < 0) {
    fprintf(stderr, "Error: Cannot open file %s\n", outFile);
    exit(1);
  }

  // generate header for the sorted file
  int recordsLeft;
  int inrc = read(infd, &recordsLeft, sizeof(recordsLeft));

  if (inrc != sizeof(recordsLeft)) {
    perror("read");
    exit(1);
  }

  int outrc = write(outfd, &recordsLeft, sizeof(recordsLeft));
  if (outrc != sizeof(recordsLeft)) {
    perror("write");
    exit(1);
  }

  if (recordsLeft == 0) {
    exit(0);
  }

  rec_dataptr_t *r = malloc(sizeof(rec_dataptr_t) * recordsLeft);
  if (r == NULL) {
    fprintf(stderr, "malloc failed\n");
    exit(1);
  }
  // Read the lines in and write
  int i;
  for (i = 0; i < recordsLeft; i++) {
    outrc = read(infd, &r[i], sizeof(rec_nodata_t));
    if (outrc != sizeof(rec_nodata_t)) {
      perror("read");
      exit(1);
    }

    (r[i]).data_ptr = malloc((r[i]).data_ints * 4);
    if ((r[i]).data_ptr == NULL) {
      fprintf(stderr, "malloc failed\n");
      exit(1);
    }
    outrc = read(infd, (r[i]).data_ptr, (r[i]).data_ints*4);
    if (outrc != (r[i]).data_ints*4) {
      perror("read");
      exit(1);
    }
  }

  qsort(r, recordsLeft, sizeof(rec_dataptr_t), compare);

  for (i = 0; i < recordsLeft; i++) {
    outrc = write(outfd, &r[i], sizeof(rec_nodata_t));
    if (outrc != sizeof(rec_nodata_t)) {
      perror("write");
      exit(1);
    }

    outrc = write(outfd, (r[i]).data_ptr, (r[i]).data_ints*4);
    if (outrc != (r[i]).data_ints*4) {
      perror("write");
      exit(1);
    }
    free((r[i]).data_ptr);
  }
  free(r);
  (void) close(infd);
  (void) close(outfd);
  return 0;
}






