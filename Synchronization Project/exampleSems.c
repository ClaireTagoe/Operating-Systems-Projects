/* Example of using semaphores - MAC compatible methods only
 * sem_open, sem_wait, sem_post
 * Author: Sherri Goings
 * Last modified: 4/18/2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

// functions for the 2 types of threads, one that produces oxygen and one
// hydrogen
void* Oxygen(void* args);
void* Hydrogen(void* args);

// function that does N^2 ops - used to introduce randomness in the running
// order of threads
void delay(int);

// declare hydrogen semaphore as global variable so shared and accessible from
// both threads
sem_t* hydro_sem;

int main() {

  // create the hydrogen semaphore, very important to use last 3 arguments as
  // shown here
  // first argument is simply filename for semaphore, any name is fine but must
  // be a valid path
  hydro_sem = sem_open("hydrosmphr", O_CREAT | O_EXCL, 0466, 0);

  // *** opening semaphores using C on a unix system creates an actual semaphore
  // file that is not
  // automatically closed or deleted when the program exits.  As long as you
  // close the semaphore AND
  // unlink the filename you gave in sem_open, you won't have any problems, but
  // if you forget, or if
  // your program crashes in the middle or you have to quit using ctrl-c or
  // something similar, you
  // will get an error when you try to run your program again because the
  // semaphore file will already
  // exist. ***
  // The following code handles the above issue by deleting the sempahore file
  // if it already existed
  // and then creating a new one.  It also handles issues where you are not
  // allowed to create/open a
  // new file, e.g. you do not have permission at the given location.
  while (hydro_sem == SEM_FAILED) {
    if (errno == EEXIST) {
      printf("semaphore hydrosmphr already exists, unlinking and reopening\n");
      fflush(stdout);
      sem_unlink("hydrosmphr");
      hydro_sem = sem_open("hydrosmphr", O_CREAT | O_EXCL, 0466, 0);
    } else {
      printf("semaphore could not be opened, error # %d\n", errno);
      fflush(stdout);
      exit(1);
    }
  }

  // create 6 threads, 4 running the Hydrogen function and 2 running the Oxygen
  // function
  pthread_t oxy1, hydro1, oxy2, hydro2, hydro3, hydro4;
  pthread_create(&hydro1, NULL, Hydrogen, NULL);
  pthread_create(&hydro2, NULL, Hydrogen, NULL);
  pthread_create(&oxy1, NULL, Oxygen, NULL);
  pthread_create(&oxy2, NULL, Oxygen, NULL);
  pthread_create(&hydro3, NULL, Hydrogen, NULL);
  pthread_create(&hydro4, NULL, Hydrogen, NULL);

  // this main program thread will wait until all 6 spawned threads exit before
  // continuing
  pthread_join(hydro1, NULL);
  pthread_join(hydro2, NULL);
  pthread_join(oxy1, NULL);
  pthread_join(oxy2, NULL);
  pthread_join(hydro3, NULL);
  pthread_join(hydro4, NULL);

  // important to BOTH close the semaphore object AND unlink the semaphore file
  sem_close(hydro_sem);
  sem_unlink("hydrosmphr");

  return 0;
}

/*
 * Produces an O atom after a random delay, checks if 2 H atoms have already
 * been
 * produced, if not waits for them to be produced, then creates H2O molecule and
 * exits
 * uses no arguments, always returns 0
 */
void* Oxygen(void* args) {

  // produce an oxygen atom, takes a random amount of work with an upper bound
  delay(rand() % 5000);
  printf("oxygen produced\n");
  fflush(stdout);

  // oxygen waits (calls down) twice on the hydrogen semaphore
  // meaning it cannot continue until at least 2 hydrogen atoms
  // have been produced
  int err = sem_wait(hydro_sem);
  int err2 = sem_wait(hydro_sem);
  if (err == -1 || err2 == -1)
    printf("error on oxygen wait for hydro_sem, error # %d\n", errno);

  // if here, know 2 hydrogen atoms have been made already so produce a water
  // molecule
  printf("made H20\n");
  fflush(stdout);

  // printf("oxygen exit\n");
  // fflush(stdout);
  return (void*)0;
}

/*
 * Produces an H atom after a random delay, notifies sem that another H is here,
 * exits
 * no arguments, always returns 0
 */
void* Hydrogen(void* args) {

  // produce a hydrogen atom, takes a random amount of work with an upper bound
  delay(rand() % 5000);
  printf("hydrogen produced\n");
  fflush(stdout);

  // post (call up) on hydrogen semaphore to signal that a hydrogen atom
  // has been produced, then immediately exit
  sem_post(hydro_sem);

  // printf("hydrogen exit\n");
  // fflush(stdout);
  return (void*)0;
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
