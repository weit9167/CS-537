#include "server_impl.h"
#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int fill_ptr = 0;
int use_ptr = 0;
int count = 0;

int num_buffers = 1;
int num_threads = 1;
int *buffer = 0;
pthread_t *thrd = 0;

pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void put(int value) {
  buffer[fill_ptr] = value;
  fill_ptr = (fill_ptr + 1) % num_buffers;
  count++;
}

int get() {
  int tmp = buffer[use_ptr];
  use_ptr = (use_ptr + 1) % num_buffers;
  count--;
  return tmp;
}

void *consumer(void *arg) {
  pthread_detach(pthread_self());
  while (1) {
    pthread_mutex_lock(&mutex);
    while (count == 0)
      pthread_cond_wait(&fill, &mutex);
    int connfd = get();

    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&mutex);
    requestHandle(connfd);
  }
  return NULL;
}


void getargs2(int argc, char *argv[]) {
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
  getargs2(argc, argv);

  // 2. allocate memory for buffer
  buffer = realloc(buffer, sizeof(int)*num_buffers);

  // 3. create threads, but all threads should be blocked before being used
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
  pthread_mutex_lock(&mutex);
  while (count == num_buffers)
    pthread_cond_wait(&empty, &mutex);
  put(connfd);
  pthread_cond_signal(&fill);
  pthread_mutex_unlock(&mutex);
}
