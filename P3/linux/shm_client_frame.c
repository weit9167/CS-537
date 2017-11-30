#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
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

stats_t *map_area;
stats_t *finder;

void exit_handler(int sig) {
  // new routine defined here specified by sigaction() in main
  // TODO(student): critical section begins
  sem_wait(mutex);
  // client reset its segment, or mark its segment as valid
  // so that the segment can be used by another client later.
  finder->inuse = 0;
  finder = NULL;
  sem_post(mutex);
  // critical section ends
  exit(0);
}

int main(int argc, char *argv[]) {
  struct timeval diff_start;
  if (gettimeofday(&diff_start, NULL)) {
    perror("first gettimeofday()\n");
    exit(1);
  }
  time_t starttime;
  char birth_client[25];
  time(&starttime);
  strcpy(birth_client, ctime(&starttime));
  birth_client[24] = '\0';
  // TODO(student): Signal handling
  // Use sigaction() here and call exit_handler
  struct sigaction act;
  act.sa_handler = exit_handler;

  if (sigaction(SIGINT, &act, NULL) < 0) {
    perror("sigaction");
    exit(1);
  }
  if (sigaction(SIGTERM, &act, NULL) < 0) {
    perror("sigaction");
    exit(1);
  }

  // TODO(student): Open the preexisting shared memory segment created by
  // shm_server
  int fd_shm = shm_open(SHM_NAME, O_RDWR, 0660);
  if (fd_shm == -1) {
    perror("fd_shm");
    exit(1);
  }

  map_area = (stats_t *)mmap(NULL, PAGESIZE,
  PROT_READ|PROT_WRITE, MAP_SHARED, fd_shm, 0);
  if (map_area == MAP_FAILED) {
    exit(1);
  }

  // TODO(student): point the mutex to the particular segment of the shared
  // memory page
  mutex = (sem_t *)map_area;

  // TODO(student): critical section begins
  sem_wait(mutex);
  // client searching through the segments of the page to find a valid
  // (or available) segment and then mark this segment as invalid
  struct timeval diff_curr, diff_diff;
  finder = map_area;
  int foundSeg = 0;
  for (int i = 0; i < 63; i++) {
    if ((++finder)->inuse == 0) {
      finder->inuse = 1;
      foundSeg = 1;
      finder->pid = getpid();
      strcpy(finder->birth, birth_client);
      finder->birth[24] = '\0';
      if (gettimeofday(&diff_curr, NULL)) {
        perror("second gettimeofday()\n");
        exit(1);
      }
      diff_diff.tv_sec = diff_curr.tv_sec - diff_start.tv_sec;
      diff_diff.tv_usec = diff_curr.tv_usec - diff_start.tv_usec;
      finder->elapsed_sec = (int)diff_diff.tv_sec;
      finder->elapsed_msec = (double)diff_diff.tv_usec/1000;
      break;
    }
  }
  if (foundSeg == 0) {
    exit(1);
  }
  sem_post(mutex);
  // critical section ends

  while (1) {
    // TODO(student): fill in fields in stats_t
    if (gettimeofday(&diff_curr, NULL)) {
      perror("third gettimeofday()\n");
      exit(1);
    }
    diff_diff.tv_sec = diff_curr.tv_sec - diff_start.tv_sec;
    diff_diff.tv_usec = diff_curr.tv_usec - diff_start.tv_usec;
    finder->elapsed_sec = (int)diff_diff.tv_sec;
    finder->elapsed_msec = (double)diff_diff.tv_usec/1000;
    sleep(1);
    // display the PIDs of all the active clients
    fprintf(stdout, "Active clients :");
    stats_t *itr = map_area;
    for (int j = 0; j < 63; j++) {
      if ((++itr)->inuse == 1) {
        fprintf(stdout, " %d", itr->pid);
      }
    }
    fprintf(stdout, "\n");
  }

  return 0;
}


