

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include "H2SO4.h"
void delay(int);

sem_t* hydro_sem;
sem_t* oxygen_sem;
sem_t* sulfur_sem;
sem_t* lock;
sem_t* atom;

void* oxygen(void* args) {
  /* produce oxygen, call down on oxygen semaphore and call up on the
   number of atoms. If the max of 4 oxygen atoms have been produced, wait
   for a molecule to be formed before producing more oxygen*/
  int err = sem_wait(oxygen_sem);
  delay(rand() % 5000);
  printf("oxygen produced\n");
  fflush(stdout);
  sem_post(atom);

  return (void*)0;
}
void* hydrogen(void* args) {
  /* produce oxygen, call down on hydrogen semaphore and call up on the
   number of atoms. If the max of 2 hydrogen atoms have been produced, wait
   for a molecule to be formed before producing more hydrogen */
  int err = sem_wait(hydro_sem);
  delay(rand() % 5000);
  printf("hydrogen produced\n");
  fflush(stdout);
  sem_post(atom);

  return (void*)0;
}

void* sulfur(void* args) {
  /*produce sulfur, check if we can create a molecule*/
  int err = sem_wait(sulfur_sem);
  delay(rand() % 5000);
  printf("sulfur produced\n");
  fflush(stdout);
  sem_post(atom);

  int err2 = sem_wait(lock);
  if (err == -1 || err2 == -1)
    printf("error on oxygen wait for hydro_sem, error # %d\n", errno);
  /* if we can create a molecule, create the molecule and increment
  the hydrogen semaphore twice, oxygen four times and sulfur ones.
  This means that each atom can now start producing again*/
  int err3 = sem_wait(atom);
  int err4 = sem_wait(atom);
  int err5 = sem_wait(atom);
  int err6 = sem_wait(atom);
  int err7 = sem_wait(atom);
  int err8 = sem_wait(atom);
  int err9 = sem_wait(atom);
  printf("H2SO4 produced\n");
  fflush(stdout);

  sem_post(sulfur_sem);
  sem_post(oxygen_sem);
  sem_post(oxygen_sem);
  sem_post(oxygen_sem);
  sem_post(oxygen_sem);
  sem_post(hydro_sem);
  sem_post(hydro_sem);

  sem_post(lock);

  return (void*)0;
}

void openSems() {
  // open all sems
  hydro_sem = sem_open("hydrosmphr", O_CREAT | O_EXCL, 0466, 2);
  while (hydro_sem == SEM_FAILED) {
    if (errno == EEXIST) {
      printf("semaphore hydrosmphr already exists, unlinking and reopening\n");
      fflush(stdout);
      sem_unlink("hydrosmphr");
      hydro_sem = sem_open("hydrosmphr", O_CREAT | O_EXCL, 0466, 2);
    } else {
      printf("semaphore could not be opened, error # %d\n", errno);
      fflush(stdout);
      exit(1);
    }
  }
  oxygen_sem = sem_open("oxysmphr", O_CREAT | O_EXCL, 0466, 4);
  while (oxygen_sem == SEM_FAILED) {
    if (errno == EEXIST) {
      printf("semaphore oxysmphr already exists, unlinking and reopening\n");
      fflush(stdout);
      sem_unlink("oxysmphr");
      oxygen_sem = sem_open("oxysmphr", O_CREAT | O_EXCL, 0466, 4);
    } else {
      printf("semaphore could not be opened, error # %d\n", errno);
      fflush(stdout);
      exit(1);
    }
  }
  sulfur_sem = sem_open("sulfursmphr", O_CREAT | O_EXCL, 0466, 1);
  while (sulfur_sem == SEM_FAILED) {
    if (errno == EEXIST) {
      printf("semaphore sulfursmphr already exists, unlinking and reopening\n");
      fflush(stdout);
      sem_unlink("sulfursmphr");
      sulfur_sem = sem_open("sulfursmphr", O_CREAT | O_EXCL, 0466, 1);
    } else {
      printf("semaphore could not be opened, error # %d\n", errno);
      fflush(stdout);
      exit(1);
    }
  }
  lock = sem_open("lock", O_CREAT | O_EXCL, 0466, 1);
  while (lock == SEM_FAILED) {
    if (errno == EEXIST) {
      printf("semaphore lock already exists, unlinking and reopening\n");
      fflush(stdout);
      sem_unlink("lock");
      sulfur_sem = sem_open("lock", O_CREAT | O_EXCL, 0466, 1);
    } else {
      printf("semaphore could not be opened, error # %d\n", errno);
      fflush(stdout);
      exit(1);
    }
  }
  atom = sem_open("atom", O_CREAT | O_EXCL, 0466, 0);
  while (atom == SEM_FAILED) {
    if (errno == EEXIST) {
      printf("semaphore atom already exists, unlinking and reopening\n");
      fflush(stdout);
      sem_unlink("atom");
      atom = sem_open("atom", O_CREAT | O_EXCL, 0466, 0);
    } else {
      printf("semaphore could not be opened, error # %d\n", errno);
      fflush(stdout);
      exit(1);
    }
  }
}
void closeSems() {
  // close and unlink all sems
  sem_close(hydro_sem);
  sem_unlink("hydrosmphr");
  sem_close(oxygen_sem);
  sem_unlink("oxysmphr");
  sem_close(sulfur_sem);
  sem_unlink("sulfursmphr");
  sem_close(lock);
  sem_unlink("lock");
  sem_close(atom);
  sem_unlink("atom");
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
