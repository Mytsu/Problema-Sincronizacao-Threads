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
#define TEMPO_ESPERA 2
#define MAXIMO_ENTRADAS 5

// Mutex para protecao do Monitor
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t signal_consumer = PTHREAD_COND_INITIALIZER;
pthread_cond_t signal_producer = PTHREAD_COND_INITIALIZER;

// Estrutura da vaga do estacionamento
typedef struct vaga {
  int slot; // Semaforo
  int id;  // Id da thread estacionada
} TVaga;

TVaga vaga;

// Controle da Fila de Prioridades
int fila[NUMERO_THREADS];

// Nomes das Threads, em ordem de prioridade
char* nomes[NUMERO_THREADS] = {"Girafales","Florinda","Xavier","Jean","Walter","Pinkman"};

// Funcoes locais
void espera(void* args); // Threads Funcionarios (Consumidores)
void diretor(void); // Thread Controladora de Deadlocks
void monitor(void); // Thread Monitora (Produtora)
int estacionar(void); // Funcao que pega proxima thread a entrar na vaga
void entrar_espera(int *id); // Funcao que coloca thread na fila de espera
void Limpar_Fila(void); // Inicializando fila com ids invalidos
int Impar(int id);

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
  if ((rc = pthread_create(&funcionarios[NUMERO_THREADS], NULL, monitor, NULL)))
    printf("Erro criando thread monitor.\n");
  assert(0 == rc);
  // Criando Diretor
  if ((rc = pthread_create(&funcionarios[NUMERO_THREADS+1], NULL, diretor, NULL)))
    printf("Erro criando thread diretor.\n");
  assert(0 == rc);

  // Criando threads
  for(i = 0; i < NUMERO_THREADS; i++) {
    threads_args[i] = i;
    printf("Main: criando thread %d\n", i);
    rc = pthread_create(&funcionarios[i], NULL, espera, (void*)&threads_args[i]);
    assert(0 == rc);
  }

  // Aguardando threads
  // Threads Monitor e Diretor não serão encerradas
  // atraves da funcao join
  for(i = 0; i < NUMERO_THREADS; i++) {
    rc = pthread_join(&funcionarios[i], NULL);
    printf("Erro thread %d\n", i);
    assert(0 == rc);
  }

  return(0);
}

// Funcao das threads funcionarios (consumidores)
void espera(void* id) {
  while(TRUE) {
    //sleep((rand() % 2) + 3); // Espera de 3 ~ 5 segundos
    entrar_espera((int*)id);
  }
}

// Funcao da thread monitor
void monitor(void) {
  while(TRUE) {
    // Trava de segurança da fila
    pthread_mutex_lock(&mutex);
    // Sinaliza aos funcionários de que podem entrar para a fila
    pthread_cond_signal(&signal_consumer);
    // Espera os funcionários liberarem o monitor
    pthread_cond_wait(&signal_producer, &mutex);
    // Coloca o funcionário de maior prioridade na vaga
    vaga.id = estacionar();
    if(vaga.id != -1)
      vaga.slot = TRUE;
    printf("%s estaciona para trabalhar", nomes[vaga.id]);
    pthread_mutex_unlock(&mutex);
    sleep(TEMPO_ESPERA);
    vaga.slot = FALSE;
  }
}

void diretor(void) {
  int i, aux;
  do {
    pthread_mutex_lock(&mutex);
    aux = FALSE;
    for(i = 0; i < NUMERO_THREADS; i++) {
      if(fila[i] == 0 || fila[i] == 1)
        aux = TRUE;
      else
        aux = FALSE;
      if(fila[i] == 2 || fila[i] == 3)
        aux = TRUE;
      else
        aux = FALSE;
      if(fila[i] == 4 || fila[i] == 5)
        aux = TRUE;
      else
        aux = FALSE;
    }
    if(aux) {
      fila[0] = rand() % 5;
      printf("Diretor detectou um deadlock, liberando %s", nomes[fila[0]]);
    }
    pthread_mutex_unlock(&mutex);
  } while(TRUE);
}


int estacionar(void) {
  int i;
  vaga.slot = fila[0];
  for(i = 0; i < NUMERO_THREADS; i++) {
    fila[i] = fila[i+1];
  }
  fila[NUMERO_THREADS] = -1;
  vaga.id = TRUE;
}

void entrar_espera(int *id) {
  int i, j, temp;
  if(*id == vaga.id)
    return;
  temp = *id;
  if(Impar(temp))
    temp--;
  // Trava de segurança para a fila
  pthread_mutex_lock(&mutex);
  // Sinaliza para o monitor para que o proximo funcionário possa entra na vaga
  pthread_cond_signal(&signal_producer);
  // Espera o monitor liberar outro funcionário entrar na fila
  pthread_cond_wait(&signal_consumer, &mutex);
  for(i = 0; i < NUMERO_THREADS; i++) {
    if(*id == fila[i] || *id == vaga.id)
      return;
    // Verificacao de prioridade na fila
    if(fila[i] == ((temp+2)%6) || fila[i] == ((temp+3)%6)) {
      for(j = NUMERO_THREADS-1; j >= i; j--) {
        if(fila[j] != -1)
          fila[j+1] = fila[j];
      }
      fila[i] = *id;
      printf("%s quer usar a vaga", nomes[*id]);
      return;
    }
  }
  pthread_mutex_unlock(&mutex);
}

void Limpar_Fila() {
  int i;
  for(i=0; i < (NUMERO_THREADS); i++) {
    fila[i] = -1; // ID invalido
  }
}

int Impar(int id) {
  if(id == 0)
    return(TRUE);
  else if((id % 2) == 0)
    return(FALSE);
  else
    return(TRUE);
}
