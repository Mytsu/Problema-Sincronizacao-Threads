#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>

#define NUMERO_THREADS 3
#define TRUE 1
#define FALSE 0

int vet[NUMERO_THREADS + 1];
char nomes[NUMERO_THREADS] = {
    "Girafales",
    "Florinda",
    "Xavier",
    "Jean",
    "Walter",
    "Pinkman"
};

int main(void) {
  pthreads_t employees[NUMERO_THREADS];
  int threads_args[NUMERO_THREADS];
  int rc, i;

  srand((unsigned)time(NULL));

  for(i = 0; i < NUMERO_THERADS; i++) {
    threads_args[i] = i;
    rc = pthread_create(&employees[i], NULL, espera, ((void)rand(), (void*)&threads_args[i]));
    assert(0 == rc);
  }

  return(0);
}

void espera(void* args) {
  while(TRUE) {
    sleep((int)args[0] % 10);
    entrar_espera((int*)args[1]);
  }
}
