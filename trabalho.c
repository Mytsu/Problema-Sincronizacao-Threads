#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>

#define NUMERO_THREADS 4
#define MAX_FUNCIONARIOS 6
#define TRUE 1
#define FALSE 0
#define TEMPO_ESPERA 5

// Vetor de Threads
int vet[NUMERO_THREADS + 1];

// Estrutura da vaga do estacionamento
typedef struct vaga {
  int slot;
  int id;
} TVaga;

TVaga vaga;

// Controle da Fila de Espera
int fila[MAX_FUNCIONARIOS + 1];
int in, out;

// Nomes das Threads, em ordem de prioridade
char* nomes[MAX_FUNCIONARIOS] = {
      "", // id reservado para a thread monitor
      "Girafales",
      "Florinda",
      "Xavier",
      "Jean",
      "Walter",
      "Pinkman"
};

// Funcoes locais
void espera(void* args);
void entrar_espera();
void Limpar_Fila();


int main(void) {
  // Variaveis de controle das threads
  pthread_t employees[NUMERO_THREADS];
  int threads_args[NUMERO_THREADS];
  int rc, i;

  // Inicio da semente
  srand((unsigned)time(NULL));

  // Iniciando estacionamento
  vaga.slot = FALSE;

  // Controle da Fila
  Limpar_Fila();

  // Criando threads
  // Thread 0 sera usada como monitor (produtor)
  for(i = 1; i < NUMERO_THREADS; i++) {
    threads_args[i] = i;
    rc = pthread_create(&employees[i], NULL, espera, (void*)&threads_args[i]);
    assert(0 == rc);
  }

  return(0);
}

// Funcao de espera das threads
void espera(void* id) {
  while(TRUE) {
    sleep(rand() % 10);
    entrar_espera((int*)id);
  }
}

// Funcao da thread monitor
void monitor(void) {
  while(TRUE) {
    if(vaga.slot == TRUE) {
      sleep(TEMPO_ESPERA);
      vaga.slot = FALSE;
    }
    vaga.id = prox_estacionar();
    vaga.slot = TRUE;
  }
}

void entrar_espera(int thread) {
  int i;
  // for inicia na posicao 1 devido ao slot do monitor
  for(i = 1; i < (MAX_FUNCIONARIOS + 1); i++) {
    if(thread == fila[i]) {
      return;
    }
  }
}

void Limpar_Fila() {
  int i;
  for(i=0; i < (MAX_FUNCIONARIOS + 1); i++) {
    fila[i] = 0;
  }
  in = 0;
  out = 0;
}
