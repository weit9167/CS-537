#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#define SHM_NAME "weit_yusong"
#define PAGESIZE 4096


// TODO(student): Include necessary headers

sem_t * mutex;

typedef struct {
  int pid;
  char birth[25];
  int elapsed_sec;
  double elapsed_msec;
  int inuse;
} stats_t;

stats_t * map_area;

void exit_handler(int sig) {
  // TODO(student): Clear the shared memory segment
  munmap(map_area, PAGESIZE);
  shm_unlink(SHM_NAME);
  exit(0);
}

int main(int argc, char **argv) {
  // TODO(student): Signal handling
  // Use sigaction() here and call exit_handler
  struct sigaction act;
  act.sa_handler = exit_handler;

  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);

  // TODO(student): Create a new shared memory segment
  int fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0660);
  if (fd_shm < 0) {
    perror("shm_open() in server\n");
    exit(1);
  }

  if (ftruncate(fd_shm, PAGESIZE) < 0) {
    perror("ftruncate() in server\n");
    exit(1);
  }

  map_area = (stats_t *)mmap(NULL, PAGESIZE,
  PROT_READ|PROT_WRITE, MAP_SHARED, fd_shm, 0);
  if (map_area == MAP_FAILED) {
    perror("mmap() in server\n");
    exit(1);
  }
  mutex = (sem_t *)map_area;

  // TODO(student): Point the mutex to the segment of the shared memory page
  // that you decide to store the semaphore
  sem_init(mutex, 1, 1);  // Initialize semaphore
  stats_t * initer;
  initer = map_area;

  for (int i = 0; i < 63; i++) {
    initer++;
    initer->inuse = 0;
  }

  // TODO(student): Some initialization of the rest of the segments in the
  // shared memory page here if necessary

  while (1) {
    initer = map_area;
    // TODO(student): Display the statistics of active clients, i.e. valid
    // segments after some formatting
    for (int j = 0; j < 63; j++) {
      if ((++initer)->inuse == 1) {
        fprintf(stdout, "pid : %d, ", initer->pid);
        fprintf(stdout, "birth : %s, ", initer->birth);
        fprintf(stdout, "elapsed : %d s", initer->elapsed_sec);
        fprintf(stdout, " %.4f ms\n", initer->elapsed_msec);
      }
    }
    sleep(1);
  }

  return 0;
}

