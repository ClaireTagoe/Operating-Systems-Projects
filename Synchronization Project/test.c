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
pthread_mutex_t children_in_oahu_lock;
pthread_mutex_t adults_in_oahu_lock;
pthread_mutex_t people_in_molokai_lock;
// pthread_mutex_t boat_Oahu_lock;
// pthread_mutex_t boat_Molokai_lock;

pthread_cond_t child_waiting_oahu;
pthread_cond_t adult_waiting_oahu;
pthread_cond_t boat_waiting;
pthread_cond_t boat_queue;
pthread_cond_t child_waiting_molokai;
// pthread_cond_t boat_molokai;
// pthread_cond_t boat_oahu;

int num_children_in_oahu;
int num_adult_in_boat;
int num_chidren_in_boat;
int num_adult_in_oahu;
int num_people_in_molokai;
int done = 0;

// int boat_at_Oahu = 1;
// int boat_at_Molokai = 0;

int main(int argc, char* argv[]) {
  /* code */
  if (argc != 3) {
    printf(
        "\nmust include num child and adult as command line arguments(e.g. "
        "'./a.out 5 6')\n\n");
    exit(1);
  }
  const int num_child = atoi(argv[1]);
  const int num_adult = atoi(argv[2]);
  total = num_adult + num_child;
  initSynch();
  // int order[total];
  int i;
  // for (i = 0; i < num_child; i++) {
  //   order[i] = 1;
  // }
  // for (; i < total; i++) {
  //   order[i] = 2;
  // }
  // printf("total: %d\n", total);
  pthread_t human[total];
  for (i = 0; i < num_adult; i++) {
    pthread_create(&human[i], NULL, adult, NULL);
  }
  for (i; i < total; i++) {
    pthread_create(&human[i], NULL, child, NULL);
  }

  for (i = 0; i < total; i++) {
    pthread_join(human[i], NULL);
  }
  sem_wait(finish);
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

begin:
  // // Check if the boat is at Oahu
  // pthread_mutex_lock(&boat_Oahu_lock);
  // while (!boat_at_Oahu) {
  //   pthread_cond_wait(&boat_oahu, &boat_Oahu_lock);
  // }
  // pthread_mutex_unlock(&boat_Oahu_lock);
  // try to get on boat
  pthread_mutex_lock(&boat_lock);
  while (num_chidren_in_boat > 1 || num_adult_in_boat > 0) {
    pthread_cond_wait(&child_waiting_oahu, &boat_lock);
  }
  num_chidren_in_boat++;
  printf("Child in boat to Molokai\n");
  fflush(stdout);
  pthread_mutex_unlock(&boat_lock);
  // Decrement number of children in Oahu
  pthread_mutex_lock(&children_in_oahu_lock);
  num_children_in_oahu--;
  pthread_mutex_unlock(&children_in_oahu_lock);
  // wait for another child to get into the boat
  pthread_mutex_lock(&boat_lock);
  while (num_chidren_in_boat < 2) {
    pthread_cond_wait(&boat_queue, &boat_lock);
  }
  pthread_cond_signal(&boat_queue);
  // pthread_mutex_unlock(&boat_lock);

  // // row boat from Oahu to Molokai
  // pthread_mutex_lock(&boat_Oahu_lock);
  // boat_at_Oahu--;
  // printf("Child rowing from Oahu to Molokai\n");
  // fflush(stdout);
  // pthread_mutex_unlock(&boat_Oahu_lock);

  // // boat arrived at molokai
  // pthread_mutex_lock(&boat_Molokai_lock);
  // boat_at_Molokai++;
  // pthread_cond_broadcast(&boat_molokai);
  // pthread_mutex_unlock(&boat_Molokai_lock);

  // arrive at molokai and get off boat
  // pthread_mutex_lock(&boat_lock);
  num_chidren_in_boat--;
  pthread_mutex_unlock(&boat_lock);

  // increment number of people in molokai
  pthread_mutex_lock(&people_in_molokai_lock);
  printf("Child arrived at Molokai\n");
  fflush(stdout);
  num_people_in_molokai++;
  printf("num ppl in m: %d\n", num_people_in_molokai);
  fflush(stdout);
  // if there are still more people on Oahu, child must wait to go back
  if (num_people_in_molokai < (total - 2)) {
    printf("unlocking molo people\n");
    pthread_mutex_unlock(&people_in_molokai_lock);
    // // check if there is a boat in molokai
    // pthread_mutex_lock(&boat_Molokai_lock);
    // while (!boat_at_Molokai) {
    //   pthread_cond_wait(&boat_molokai, &boat_Molokai_lock);
    // }
    // pthread_mutex_unlock(&boat_Molokai_lock);

    // check if there is already a child going back to Oahu
    pthread_mutex_lock(&boat_lock);
    while (num_chidren_in_boat > 0) {
      pthread_cond_wait(&child_waiting_molokai, &boat_lock);
    }

    // if there aren't any more people, don't go back, child exits
    if (done) {
      goto end;
    }
    // else, child rows a boat back to Oahu
    num_chidren_in_boat++;
    // pthread_mutex_unlock(&boat_lock);
    // // decrement the number of boats in molokai
    // pthread_mutex_lock(&boat_Molokai_lock);
    // boat_at_Molokai--;
    // printf("Child rowing boat back to Oahu from Molokai\n");
    // fflush(stdout);
    // pthread_mutex_unlock(&boat_Molokai_lock);

    // // decrement the number of boats in molokai
    // pthread_mutex_lock(&boat_Oahu_lock);
    // boat_at_Oahu++;
    // printf("Boat arrives at Oahu with 1 child. num ppl in m: %d\n",
    //        num_people_in_molokai);
    // fflush(stdout);
    // pthread_cond_broadcast(&boat_oahu);
    // pthread_mutex_unlock(&boat_Oahu_lock);

    // Child gets off the boat after arrived at Oahu
    // pthread_mutex_lock(&boat_lock);
    num_chidren_in_boat--;
    pthread_mutex_unlock(&boat_lock);

    // arrives at Oahu
    pthread_mutex_lock(&children_in_oahu_lock);
    num_children_in_oahu++;
    printf("Child gets off the boat and arrives at Oahu\n");
    fflush(stdout);
    pthread_mutex_unlock(&children_in_oahu_lock);
    goto begin;

  } else {
    pthread_mutex_unlock(&people_in_molokai_lock);
    printf("All people are on Molokai, %d\n", num_people_in_molokai);
    fflush(stdout);
    pthread_cond_broadcast(&child_waiting_molokai);
    sem_post(finish);
  }
end:
  ;
}

void* adult(void* args) {
  /* code */
  printf("1 Adult has arrived on Oahu\n");
  fflush(stdout);
  count += 1;
  if (count != total) {
    sem_wait(start);
  }
  sem_post(start);

  // adult has to wait for all chidren to be moved to Molokai
  pthread_mutex_lock(&children_in_oahu_lock);
  while (num_children_in_oahu > 2) {
    pthread_cond_wait(&adult_waiting_oahu, &children_in_oahu_lock);
  }
  pthread_mutex_unlock(&children_in_oahu_lock);
  pthread_mutex_lock(&boat_lock);
  // // Check if the boat is at Oahu
  // pthread_mutex_lock(&boat_Oahu_lock);
  // while (!boat_at_Oahu) {
  //   pthread_cond_wait(&boat_oahu, &boat_Oahu_lock);
  // }
  // pthread_mutex_unlock(&boat_Oahu_lock);
  // check if the boat is full or if there is at least one child
  while (num_adult_in_boat + num_chidren_in_boat > 0) {
    pthread_cond_wait(&boat_waiting, &boat_lock);
  }
  // board the boat
  num_adult_in_boat++;
  printf("Adult is on boat\n");
  fflush(stdout);
  // decrement number of adults in Oahu
  pthread_mutex_lock(&adults_in_oahu_lock);
  num_adult_in_oahu--;
  // pthread_mutex_unlock(&adults_in_oahu_lock);
  // // row boat from Oahu to Molokai
  // pthread_mutex_lock(&boat_Oahu_lock);
  // boat_at_Oahu--;
  // printf("Adult rowing from Oahu to Molokai\n");
  // fflush(stdout);
  // pthread_mutex_unlock(&boat_Oahu_lock);
  //
  // // boat arrived at molokai
  // pthread_mutex_lock(&boat_Molokai_lock);
  // boat_at_Molokai++;
  // pthread_cond_broadcast(&boat_molokai);
  // pthread_mutex_unlock(&boat_Molokai_lock);

  // Decrement number of people in boat(exit boat)
  // pthread_mutex_lock(&boat_lock);
  num_adult_in_boat--;
  pthread_mutex_unlock(&boat_lock);

  // get onto island and increment number of people on island
  pthread_mutex_lock(&people_in_molokai_lock);
  num_people_in_molokai++;
  printf("Adult is on Molokai\n");
  fflush(stdout);
  // wake up all sleeping children waiting to pick up people from Oahu
  pthread_cond_broadcast(&child_waiting_molokai);
  pthread_mutex_unlock(&people_in_molokai_lock);
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
  pthread_mutex_init(&children_in_oahu_lock, NULL);
  pthread_mutex_init(&adults_in_oahu_lock, NULL);
  pthread_mutex_init(&people_in_molokai_lock, NULL);
  // pthread_mutex_init(&boat_Molokai_lock, NULL);
  // pthread_mutex_init(&boat_Oahu_lock, NULL);

  pthread_cond_init(&child_waiting_oahu, NULL);
  pthread_cond_init(&adult_waiting_oahu, NULL);
  pthread_cond_init(&boat_waiting, NULL);
  pthread_cond_init(&boat_queue, NULL);
  pthread_cond_init(&child_waiting_molokai, NULL);
  // pthread_cond_init(&boat_molokai, NULL);
  // pthread_cond_init(&boat_oahu, NULL);
}

void close() {
  sem_close(start);
  sem_unlink("startsmphr");
  sem_close(finish);
  sem_unlink("finishsmphr");

  pthread_mutex_destroy(&boat_lock);
  pthread_mutex_destroy(&children_in_oahu_lock);
  pthread_mutex_destroy(&adults_in_oahu_lock);
  pthread_mutex_destroy(&people_in_molokai_lock);
  // pthread_mutex_destroy(&boat_Molokai_lock);
  // pthread_mutex_destroy(&boat_Oahu_lock);

  pthread_cond_destroy(&child_waiting_oahu);
  pthread_cond_destroy(&adult_waiting_oahu);
  pthread_cond_destroy(&boat_waiting);
  pthread_cond_destroy(&boat_queue);
  pthread_cond_destroy(&child_waiting_molokai);
  // pthread_cond_destroy(&boat_oahu);
  // pthread_cond_destroy(&boat_molokai);
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
