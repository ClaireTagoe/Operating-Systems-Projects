#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>

int count = 0;
int total;

void delay(int);
void* child(void* args);
void* adult(void* args);
void initSynch();
void close();

// Locks and Condition Variables
sem_t* start;
sem_t* finish;

pthread_mutex_t boat_lock;

pthread_cond_t child_waiting_oahu;
pthread_cond_t adult_waiting_oahu;
pthread_cond_t child_waiting_molokai;
pthread_cond_t waittorow;
pthread_cond_t boat_empty;

int num_children_in_oahu;
int num_adult_in_boat = 0;
int num_chidren_in_boat = 0;
int num_adult_in_oahu;
int num_people_in_molokai = 0;
int done = 0;

int boat_at_Oahu = 1;
int boat_at_Molokai = 0;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf(
        "\nmust include num child and adult as command line arguments(e.g. "
        "'./a.out 5 6')\n\n");
    exit(1);
  }
  const int num_child = atoi(argv[1]);
  const int num_adult = atoi(argv[2]);
  total = num_adult + num_child;
  num_adult_in_oahu = num_adult;
  num_children_in_oahu = num_child;

  initSynch();
  int i;
  pthread_t human[total];
  for (i = 0; i < num_adult; i++) {
    pthread_create(&human[i], NULL, adult, NULL);
  }
  for (; i < total; i++) {
    pthread_create(&human[i], NULL, child, NULL);
  }

  for (i = 0; i < total; i++) {
    pthread_join(human[i], NULL);
  }
  sem_wait(finish);
  printf("Everyone arrives on Molokai!\n");
  fflush(stdout);
  close();
}

void* child(void* args) {
  /* code */
  printf("1 Child has arrived on Oahu\n");
  fflush(stdout);
  count += 1;
  if (count != total) {

    sem_wait(start);
  }
  sem_post(start);

  while (1) {
    // Check if the boat is full and at Oahu
    pthread_mutex_lock(&boat_lock);
    while ((num_chidren_in_boat > 1 || num_adult_in_boat > 0) ||
           !boat_at_Oahu ||
           (num_children_in_oahu < 2 && num_adult_in_oahu > 0 &&
            num_chidren_in_boat == 0)) {
      pthread_cond_wait(&child_waiting_oahu, &boat_lock);
    }

    num_chidren_in_boat++;
    num_children_in_oahu--;
    printf("Child getting on the boat on Oahu\n");
    fflush(stdout);

    pthread_cond_signal(&waittorow);
    // waiting for the other child to get on the boat before rowing
    while (num_chidren_in_boat < 2 && boat_at_Oahu) {
      pthread_cond_wait(&waittorow, &boat_lock);
    }

    boat_at_Oahu = 0;
    pthread_mutex_unlock(&boat_lock);

    // rowing from O to M
    printf("Child rowing the boat from Oahu to Molokai\n");
    fflush(stdout);

    // child arrives at Molokai and unboard
    pthread_mutex_lock(&boat_lock);
    boat_at_Molokai = 1;
    num_people_in_molokai++;
    num_chidren_in_boat--;
    printf("Child getting off the boat on Molokai\n");
    fflush(stdout);
    pthread_cond_signal(&boat_empty);

    // wait for both children to get off the boat
    while (num_chidren_in_boat > 0 && boat_at_Molokai) {
      pthread_cond_wait(&boat_empty, &boat_lock);
    }
    pthread_mutex_unlock(&boat_lock);

    // if all people arrive on Molokai
    if (num_people_in_molokai == total) {
      done = 1;
      pthread_cond_broadcast(&child_waiting_molokai);
      sem_post(finish);
      break;
    }
    // waking up all children on molokai that the boat is here
    pthread_cond_broadcast(&child_waiting_molokai);

    // Check if there is child on the boat back to Oahu and the boat is at
    // molokai
    pthread_mutex_lock(&boat_lock);
    while (num_chidren_in_boat > 0 || !boat_at_Molokai) {
      pthread_cond_wait(&child_waiting_molokai, &boat_lock);
    }
    if (done) {
      pthread_mutex_unlock(&boat_lock);
      break;
    }

    num_chidren_in_boat++;
    num_people_in_molokai--;
    printf("Child getting on the boat on Molokai\n");
    fflush(stdout);
    boat_at_Molokai = 0;
    printf("Child rowing from Molokai to Oahu\n");
    fflush(stdout);
    boat_at_Oahu = 1;
    num_chidren_in_boat--;
    num_children_in_oahu++;
    printf("Child getting off the boat on Oahu\n");
    fflush(stdout);
    pthread_cond_broadcast(&adult_waiting_oahu);
    pthread_cond_broadcast(&child_waiting_oahu);

    pthread_mutex_unlock(&boat_lock);
  }
  return (void*)0;
}

void* adult(void* args) {
  printf("1 Adult has arrived on Oahu\n");
  fflush(stdout);
  count += 1;
  if (count != total) {
    sem_wait(start);
  }
  sem_post(start);

  // adult has to wait for all chidren to be moved to Molokai
  pthread_mutex_lock(&boat_lock);

  while (num_children_in_oahu > 1 ||
         (num_adult_in_boat + num_chidren_in_boat > 0) || !boat_at_Oahu) {
    pthread_cond_wait(&adult_waiting_oahu, &boat_lock);
  }

  // board the boat
  num_adult_in_boat++;
  printf("Adult getting on the boat on Oahu\n");
  fflush(stdout);
  num_adult_in_oahu--;

  // row boat from Oahu to Molokai

  boat_at_Oahu = 0;
  printf("Adult rowing from Oahu to Molokai\n");
  fflush(stdout);

  // boat arrived at molokai
  boat_at_Molokai = 1;
  num_adult_in_boat--;
  num_people_in_molokai++;
  printf("Adult getting off the boat on Molokai\n");
  fflush(stdout);

  // wake up all sleeping children waiting to pick up people from Oahu
  pthread_cond_broadcast(&child_waiting_molokai);
  pthread_mutex_unlock(&boat_lock);
  return (void*)0;
}

void initSynch() {
  start = sem_open("startsmphr", O_CREAT | O_EXCL, 0466, 0);
  while (start == SEM_FAILED) {
    if (errno == EEXIST) {
      printf("semaphore start already exists, unlinking and reopening\n");
      fflush(stdout);
      sem_unlink("startsmphr");
      start = sem_open("startsmphr", O_CREAT | O_EXCL, 0466, 0);
    } else {
      printf("semaphore could not be opened, error # %d\n", errno);
      fflush(stdout);
      exit(1);
    }
  }
  finish = sem_open("finishsmphr", O_CREAT | O_EXCL, 0466, 0);
  while (finish == SEM_FAILED) {
    if (errno == EEXIST) {
      printf("semaphore finish already exists, unlinking and reopening\n");
      fflush(stdout);
      sem_unlink("finishsmphr");
      finish = sem_open("finishsmphr", O_CREAT | O_EXCL, 0466, 0);
    } else {
      printf("semaphore could not be opened, error # %d\n", errno);
      fflush(stdout);
      exit(1);
    }
  }

  pthread_mutex_init(&boat_lock, NULL);

  pthread_cond_init(&boat_empty, NULL);
  pthread_cond_init(&waittorow, NULL);
  pthread_cond_init(&child_waiting_oahu, NULL);
  pthread_cond_init(&adult_waiting_oahu, NULL);
  pthread_cond_init(&child_waiting_molokai, NULL);
}

void close() {
  sem_close(start);
  sem_unlink("startsmphr");
  sem_close(finish);
  sem_unlink("finishsmphr");

  pthread_mutex_destroy(&boat_lock);

  pthread_cond_destroy(&boat_empty);
  pthread_cond_destroy(&waittorow);
  pthread_cond_destroy(&child_waiting_oahu);
  pthread_cond_destroy(&adult_waiting_oahu);
  pthread_cond_destroy(&child_waiting_molokai);
}
/*
 * NOP function to simply use up CPU time
 * arg limit is number of times to run each loop, so runs limit^2 total loops
 */
void delay(int limit) {
  int j, k;

  for (j = 0; j < limit; j++) {
    for (k = 0; k < limit; k++) {
    }
  }
}
