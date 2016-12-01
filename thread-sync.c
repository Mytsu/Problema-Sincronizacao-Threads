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
#define MAXIMO_ENTRADAS 5
#define TEMPO_ESPERA_DIRETOR 2
#define TEMPO_ESPERA_MONITOR 2
#define TEMPO_ESPERA_FUNCIONARIO 0

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

// Controle de entradas de cada Funcionarios
int entradas[NUMERO_THREADS];

//int i; Funcoes locais
void* espera(void* args); // Threads Funcionarios (Consumidores)
void* diretor(void* args); // Thread Controladora de Deadlocks
void* monitor(void* args); // Thread Monitora (Produtora)
void Iniciar_Entradas(void); // Inicia o vetor de entradas
void Limpar_Fila(void); // Inicializando fila com ids invalidos
int Checar_Entradas(void);
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

  // Iniciando vetor de entradas pra contagem de cada funcionario
  Iniciar_Entradas();

  // Setando estacionamento para vago

  vaga.slot = FALSE;
  vaga.id = -1;

  // Criando Monitor
  if (pthread_create(&funcionarios[NUMERO_THREADS], NULL, monitor, NULL)) {
    printf("Erro criando thread monitor.\n");
    return(1);
  }
  // Criando Diretor
  if (pthread_create(&funcionarios[NUMERO_THREADS+1], NULL, diretor, NULL)) {
    printf("Erro criando thread diretor.\n");
    return(1);
  }

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
    rc = pthread_join(funcionarios[i], NULL);
    assert(0 == rc);
  }
  return(0);
}

// Funcao das threads funcionarios (consumidores)
void* espera(void* args) {
  int id = *(int *)args;
  while(!Checar_Entradas()) {
    sleep(TEMPO_ESPERA_FUNCIONARIO); // Espera de 3 ~ 5 segundos
    int i, j, temp;
    if(id == vaga.id)
      continue;
    temp = id;
    if(Impar(temp))
      temp--;
    for(i = 0; i < NUMERO_THREADS; i++) {
      if(fila[i] == id) {
        continue;
      }
    }
    // Trava de segurança para a fila
    pthread_mutex_lock(&mutex);
    // Sinaliza para o monitor para que o proximo funcionário possa entra na vaga
    pthread_cond_signal(&signal_producer);
    // Espera o monitor liberar outro funcionário entrar na fila
    pthread_cond_wait(&signal_consumer, &mutex);
    for(i = 0; i < NUMERO_THREADS; i++) {
      // Verificacao de prioridade na fila
      if((fila[i] == ((temp+2)%6) || fila[i] == ((temp+3)%6)) || fila[i] == -1) {
        for(j = NUMERO_THREADS-1; j >= i; j--) {
          if(fila[j] != -1)
            fila[j+1] = fila[j];
        }
        fila[i] = id;
        printf("%s quer usar a vaga\n ", nomes[id]);
        break;
      }
    }
    pthread_mutex_unlock(&mutex);
  }
}

// Funcao da thread monitor
void* monitor(void* args) {
  while(!Checar_Entradas()) {
    int i;
    // Trava de segurança da fila
    pthread_mutex_lock(&mutex);
    // Sinaliza aos funcionários de que podem entrar para a fila
    pthread_cond_signal(&signal_consumer);
    // Espera os funcionários liberarem o monitor
    pthread_cond_wait(&signal_producer, &mutex);
    // Coloca o funcionário de maior prioridade na vaga
    if((!vaga.slot) && (fila[0] != -1)) {
      vaga.id = fila[0];
      for(i = 0; i < NUMERO_THREADS; i++) {
        fila[i] = fila[i+1];
      }
      fila[NUMERO_THREADS] = -1;
      if(vaga.id != -1) {
        vaga.slot = TRUE;
        entradas[vaga.id] = entradas[vaga.id] + 1;
        printf("%s estaciona para trabalhar\n", nomes[vaga.id]);
      }
    }
    sleep(TEMPO_ESPERA_MONITOR);
    if(vaga.slot) {
      vaga.slot = FALSE;
      printf("%s vai para casa estudar\n", nomes[vaga.id]);
    }
    pthread_mutex_unlock(&mutex);
  }
}

void* diretor(void* args) {
  int i, temp, aux1, aux2, aux3;
  do {
    sleep(TEMPO_ESPERA_DIRETOR);
    pthread_mutex_lock(&mutex);
    if(vaga.slot) {
      continue;
    }
    aux1 = aux2 = aux3 = FALSE;
    for(i = 0; i < NUMERO_THREADS; i++) {
      if(fila[i] == 0 || fila[i] == 1)
        aux1 = TRUE;
      if(fila[i] == 2 || fila[i] == 3)
        aux2 = TRUE;
      if(fila[i] == 4 || fila[i] == 5)
        aux3 = TRUE;
    }
    if(aux1 && aux2 && aux3) {
      temp = rand() % 5;
      printf("Diretor detectou um deadlock, liberando %s\n", nomes[fila[0]]);
      for(i = 1; i < NUMERO_THREADS; i++) {
        if(fila[i] == temp) {
          temp = fila[0];
          fila[0] = fila[i];
          fila[i] = temp;
          break;
        }
      }
    }
    pthread_mutex_unlock(&mutex);
  } while(!Checar_Entradas());
}

void Iniciar_Entradas(void) {
  int i;
  for(i = 0; i < NUMERO_THREADS; i++) {
    entradas[i] = 0;
  }
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

int Checar_Entradas(void) {
  int i;
  for(i = 0; i < NUMERO_THREADS; i++) {
    if(entradas[i] < MAXIMO_ENTRADAS)
      return(0);
  }
  return(1);
}
