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
#define MAXIMO_ENTRADAS 10
#define TEMPO_ESPERA_MONITOR 0
#define TEMPO_ESPERA_FUNCIONARIO 0

// Estrutura da vaga do estacionamento
typedef struct vaga {
  int slot; 
  int id;  // Id da thread estacionada
} TVaga;

TVaga vaga;

// Mutex para protecao do Monitor
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t signal_consumer = PTHREAD_COND_INITIALIZER;
pthread_cond_t signal_producer = PTHREAD_COND_INITIALIZER;

// Controle da Fila de Prioridades
int fila[NUMERO_THREADS];

// Nomes das Threads, em ordem de prioridade
char* nomes[NUMERO_THREADS] = {"Girafales","Florinda","Xavier","Jean","Walter","Pinkman"};

// Controle de entradas de cada Funcionarios
int entradas[NUMERO_THREADS];

// Funcoes locais
void* espera(void* args); // Threads Funcionarios (Consumidores)
void* diretor(void* args); // Thread Controladora de Deadlocks
void* monitor(void* args); // Thread Monitora (Produtora)
void Iniciar_Entradas(void); // Inicia o vetor de entradas
void Limpar_Fila(void); // Inicializando fila com ids invalidos
int Checar_Entradas(void);
int Checar_Deadlock(void);
int Impar(int id);
int Fila_Cheia(void);
int ID_presente_fila(int id);
void Verifica_fila(void);

// Inicio procedimento principal
int main(void) {
  // Variaveis de controle das threads
  pthread_t funcionarios[NUMERO_THREADS+1];
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
  // Criando threads
  for(i = 0; i < NUMERO_THREADS; i++) {
    threads_args[i] = i;
    rc = pthread_create(&funcionarios[i], NULL, espera, (void*)&threads_args[i]);
    assert(0 == rc);
  }

  // Aguardando threads
  // Threads Monitor e Diretor não serão encerradas
  // atraves da funcao join
  for(i = 0; i <= NUMERO_THREADS; i++) {
    rc = pthread_join(funcionarios[i], NULL);
    assert(0 == rc);
  }

  pthread_cond_destroy(&signal_consumer);
  pthread_cond_destroy(&signal_producer);
  return(0);
}

// Funcao das threads funcionarios (consumidores)
void* espera(void* args) {
  int id = *(int *)args;
  int aux, i, j, temp;
  do {
    sleep(TEMPO_ESPERA_FUNCIONARIO);
    temp = id;
    if(Impar(temp)) {
      temp--;
    }
    if(Fila_Cheia() || ID_presente_fila(id)) {
      pthread_cond_signal(&signal_producer);
      continue;
    }
    // Trava de segurança para a fila
    pthread_mutex_lock(&mutex);
    // Sinaliza para o monitor para que o proximo funcionário possa entra na vaga
    pthread_cond_signal(&signal_producer);
    // Espera o monitor liberar outro funcionário entrar na fila
    pthread_cond_wait(&signal_consumer, &mutex);
    if(vaga.id == temp || vaga.id == (temp+1)) {
      continue;
    }
    for(i = 0; i < NUMERO_THREADS; i++) {
      // Verificacao de prioridade na fila
      if((fila[i] == ((temp+2)%6) || fila[i] == ((temp+3)%6)) || fila[i] == -1) {
        if(temp == vaga.id || id == vaga.id) {
          break;
        }
        aux = fila[i];
        fila[i] = id;
        for(j = (i+1); j < NUMERO_THREADS; j++) {
          if(fila[j] == -1) {
            fila[j] = aux;
            aux = fila[j+1];
            break;
          }
        }
        printf("%s quer usar a vaga\n ", nomes[id]);
        break;
      }
    }
    pthread_mutex_unlock(&mutex);
  } while(entradas[id] < MAXIMO_ENTRADAS);
  return(NULL);
}

// Funcao da thread monitor
void* monitor(void* args) {
  do {
    int i;
    if(Checar_Deadlock()) {
      diretor(NULL);
    }
    // Trava de segurança da fila
    pthread_mutex_lock(&mutex);
    // Espera diretor e funcionarios liberarem o monitor
    pthread_cond_signal(&signal_consumer);
    pthread_cond_wait(&signal_producer, &mutex);
    // Coloca o funcionário de maior prioridade na vaga
    if(fila[0] != -1) {
      vaga.id = fila[0];
      for(i = 0; i < (NUMERO_THREADS-1); i++) {
        fila[i] = fila[i+1];
      }
      pthread_cond_signal(&signal_consumer);
      pthread_mutex_unlock(&mutex);
      fila[NUMERO_THREADS-1] = -1;
      vaga.slot = TRUE;
      entradas[vaga.id] = entradas[vaga.id] + 1;
      printf("%s estaciona para trabalhar\n", nomes[vaga.id]);
      sleep(TEMPO_ESPERA_MONITOR);
      vaga.slot = FALSE;
      printf("%s vai para casa estudar\n", nomes[vaga.id]);
    }
    else {
      pthread_mutex_unlock(&mutex);
    }
  } while(!Checar_Entradas());
  return(NULL);
}

void* diretor(void* args) {
  int i, temp, aux;
    pthread_mutex_lock(&mutex);
    // Espera monitor liberar verificacao
    // Verifica se ja ha um funcionario na vaga

    if(vaga.slot) {
      // libera acesso ao produtor e reinicia o loop
      pthread_mutex_unlock(&mutex);
      return(NULL);
    }
      // sorteia um funcionario dentre os presentes na fila de espera
      aux = FALSE;
      while(!aux) {
        temp = rand() % 5;
        for(i = 0; i < NUMERO_THREADS; i++) {
          if(fila[i] == temp) {
            aux = TRUE;
          }
        }
      }
      printf("Diretor detectou um deadlock, liberando %s\n", nomes[temp]);
      // coloca o funcionario no inicio da fila para que o monitor o coloque na vaga
	
	/*
	
		Bug fix: a função de manipulação da fila de prioridades do diretor 
		gerava dados inválidos devido a duplicação de ids na fila.
		Manipulação alterada para uma troca simples entre a posição 0 (fila[0])
		e a posição anterior (fila[i]) do id sorteado.
	
	*/
      for(i = 1; i < NUMERO_THREADS; i++) {
        if(fila[i] == temp) {
          temp = fila[0];
          fila[0] = fila[i];
          fila[i] = temp;
          break;
        }
      }
    // libera acesso ao produtor    
    pthread_mutex_unlock(&mutex);
  return(NULL);
}

void Iniciar_Entradas(void) {
  int i;
  for(i = 0; i < NUMERO_THREADS; i++) {
    entradas[i] = 0;
  }
}

void Limpar_Fila() {
  int i;
  for(i=0; i < NUMERO_THREADS; i++) {
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

int ID_presente_fila(int id) {
  pthread_mutex_lock(&mutex);
  int i;
  for(i = 0; i < NUMERO_THREADS; i++) {
    if(fila[i] == id) {
      pthread_mutex_unlock(&mutex);
      return(TRUE);
    }
  }
  pthread_mutex_unlock(&mutex);
  return(FALSE);
}

int Fila_Cheia(void) {
  int i;
  pthread_mutex_lock(&mutex);
  for(i = 0; i < NUMERO_THREADS; i++) {
    if(fila[i] == -1) {
      pthread_mutex_unlock(&mutex);
      return(0);
    }
  }
  pthread_mutex_unlock(&mutex);
  return(1);
}

void Verifica_fila (void) {
	int i;
  printf("-----------\n");
  for(i=0;i<NUMERO_THREADS;i++)	{
  	printf("pos %d esta na fila  %s \n",i,nomes[fila[i]]);
  }
  printf("-----------\n");
}

int Checar_Deadlock(void) {
  pthread_mutex_lock(&mutex);
  int i, aux1, aux2, aux3;
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
    pthread_mutex_unlock(&mutex);
    return(TRUE);
  }
  pthread_mutex_unlock(&mutex);
  return(FALSE);
}
