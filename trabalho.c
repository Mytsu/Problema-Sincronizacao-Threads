//=====================================================
// Arquivo: trabalho.c
//
// Autores: Jonathan Arantes Pinto
//          Rúbia Marques Oliveira
//          Ana Paula Fernandes
// Data: 11/11/2016
//
// Trabalho de Sistemas Operacionais - Sincronizacao de Threads
// 
// Instituto Federal de Minas Gerais - Campus Formiga
//=====================================================

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>

#define NUMERO_THREADS 6
#define TRUE 1
#define FALSE 0
#define TEMPO_ESPERA 5

// Mutex para protecao do Monitor
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Estrutura da vaga do estacionamento
typedef struct vaga {
  int slot; // Semaforo
  int id;  // Id da thread estacionada
} TVaga;

TVaga vaga;

// Controle da Fila de Prioridades
int fila[NUMERO_THREADS];

// Controle de quantas vezes cada thread estacionou
int

// Nomes das Threads, em ordem de prioridade
char* nomes[NUMERO_THREADS -1] = {
      "Girafales",
      "Florinda",
      "Xavier",
      "Jean",
      "Walter",
      "Pinkman"
};

// Funcoes locais
void espera(void* args); // Threads Funcionarios (Consumidores)
void diretor(void); // Thread Controladora de Deadlocks
void monitor(void); // Thread Monitora (Produtora)
int estacionar(void); // Funcao que pega proxima thread a entrar na vaga
void entrar_espera(); // Funcao que coloca thread na fila de espera
void Limpar_Fila(); // Inicializando fila com ids invalidos

// Inicio procedimento pai
int main(char* args[]) {
  // Variaveis de controle das threads
  pthread_t funcionarios[NUMERO_THREADS+2];
  // As threads extras sao:
  // 7  - Monitor (Produtor)
  // 8  - Diretor (Controlador de Deadlocks)
  int threads_args[NUMERO_THREADS];
  int rc, i;

  // Inicio da semente
  srand((unsigned)time(NULL));

  // Iniciando fila de prioridades com id invalido
  // Evitando que a thread monitor inicie de forma indevida
  Limpar_Fila();

  // Criando Monitor
  if ((rc = pthread_create(&funcionarios[6], NULL, monitor, NULL)))
    printf(stderr, "Erro criando thread monitor.\n");
  if ((rc = pthread_create(&funcionarios[7], NULL, diretor, NULL)))
    printf(stderr, "Erro criando thread diretor.\n");

  // Criando threads
  for(i = 0; i < NUMERO_THREADS; i++) {
    threads_args[i] = i;
    rc = pthread_create(&funcionarios[i], NULL, espera, (void*)&threads_args[i]);
    assert(0 == rc);
  }

  // Aguardando threads
  // Threads Monitor e Diretor não serão encerradas
  // atraves da funcao join
  for(i = 0; i < NUMERO_THREADS; i++) {
    rc = pthread(join(threads[i], NULL));
    assert(0 == rc);
  }

  return(EXIT_SUCESS);
}

// Funcao das threads funcionarios (consumidores)
void espera(void* id) {
  while(TRUE) {
    sleep((rand() % 3) + 3); // Espera de 3 ~ 5 segundos
    entrar_espera((int*)id);
  }
}

// Funcao da thread monitor
void monitor(void) {
  while(TRUE) {
    pthread_mutex_lock(&mutex);
      vaga.id = estacionar();
      if(vaga.id != -1)
        vaga.slot = TRUE;
    pthread_mutex_unlock(&mutex);
    if(vaga.id != -1)
      sleep(TEMPO_ESPERA);
    pthread_mutex_lock(&mutex);
      vaga.slot = FALSE;
    pthread_mutex_unlock(&mutex);
  }
}

void diretor(void) {
  //...
}


int estacionar(void) {
  //...
}

void entrar_espera(int id) {
  int i;
  for(i = 0; i < NUMERO_THREADS; i++) {
    if(id == fila[i] || id == vaga.id)
      return;
    // Verificacao de prioridade na fila
    // Entrada da thread na fila
    // ...
  }
}

void Limpar_Fila() {
  int i;
  for(i=0; i < (NUMERO_THREADS); i++) {
    fila[i] = -1; // ID invalido
  }
}
