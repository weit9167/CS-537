#include "server_impl.h"
#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

sem_t empty;
sem_t full;
sem_t mutex;

int num_buffers = 1;
int num_threads = 1;

int use = 0;
int fill = 0;

int *buffer = 0;
pthread_t *thrd = 0;

void put(int value) {
  buffer[fill] = value;
  fill = (fill + 1)%num_buffers;
}

int get() {
  int connfd = buffer[use];
  use = (use + 1)%num_buffers;
  return connfd;
}

void *consumer(void *arg) {
  pthread_detach(pthread_self());
  while (1) {
    sem_wait(&full);
    sem_wait(&mutex);
    int connfd = get();
    sem_post(&mutex);
    sem_post(&empty);
    requestHandle(connfd);
  }
}

void getargs3(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s [portnum] [threads] [buffers]\n", argv[0]);
    exit(1);
  }
  num_threads = atoi(argv[2]);
  if (num_threads <= 0) {
    fprintf(stderr, "threads must be a positive integer\n");
    exit(1);
  }
  num_buffers = atoi(argv[3]);
  if (num_buffers <= 0) {
    fprintf(stderr, "buffers must be a positive integer\n");
    exit(1);
  }
}


void server_init(int argc, char* argv[]) {
  // 1. check the 3rd and 4th input arguments
  getargs3(argc, argv);

  // 2. allocate memory for buffer
  buffer = realloc(buffer, sizeof(int)*num_buffers);

  // 3. initialize semaphores
  sem_init(&empty, 0, num_buffers);
  sem_init(&full, 0, 0);
  sem_init(&mutex, 0, 1);

  // 4. create threads, but all threads should be blocked before being used
  thrd = realloc(thrd, sizeof(pthread_t)*num_threads);
  int i;
  for (i = 0; i < num_threads; i++) {
    int rc = pthread_create(&thrd[i], NULL, &consumer, NULL);
    if (rc != 0) {
      fprintf(stderr, "pthread_create\n");
      exit(1);
    }
  }
}

void server_dispatch(int connfd) {
  sem_wait(&empty);
  sem_wait(&mutex);
  put(connfd);
  sem_post(&mutex);
  sem_post(&full);
}
