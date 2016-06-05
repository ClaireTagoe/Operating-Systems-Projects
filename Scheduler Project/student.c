/*
 * student.c
 * This file contains the CPU scheduler for the simulation.
 * original base code from http://www.cc.gatech.edu/~rama/CS2200
 * Last modified 5/11/2016 by Sherri Goings
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os-sim.h"
#include "student.h"

// Local helper functions
static void addReadyProcess(pcb_t* proc);
static pcb_t* getReadyProcess(void);
static void schedule(unsigned int cpu_id);

/*
 * enum is useful C language construct to associate desriptive words with
 * integer values
 * in this case the variable "alg" is created to be of the given enum type,
 * which allows
 * statements like "if alg == FIFO { ...}", which is much better than "if alg ==
 * 1" where
 * you have to remember what algorithm is meant by "1"...
 * just including it here to introduce you to the idea if you haven't seen it
 * before!
 */
typedef enum {
  FIFO = 0,
  RoundRobin,
  StaticPriority,
  MultiLevel_Queues
} scheduler_alg;

scheduler_alg alg;

// declare other global vars
int time_slice = -1;
int max_wait_time = -1;
int cpu_count;

/*
 * main() parses command line arguments, initializes globals, and starts
 * simulation
 */
int main(int argc, char* argv[]) {
  /* Parse command line args - must include num_cpus as first, rest optional
   * Default is to simulate using just FIFO on given num cpus, if 2nd arg given:
   * if -r, use round robin to schedule (must be 3rd arg of time_slice)
   * if -p, use static priority to schedule
   */
  if (argc == 2) {
    alg = FIFO;
    printf("running with basic FIFO\n");
  } else if (argc > 2 && strcmp(argv[2], "-r") == 0 && argc > 3) {
    alg = RoundRobin;
    time_slice = atoi(argv[3]);
    printf("running with round robin, time slice = %d\n", time_slice);
  } else if (argc > 2 && strcmp(argv[2], "-p") == 0) {
    alg = StaticPriority;
    printf("running with static priority\n");
  } else if (argc > 4 && strcmp(argv[2], "-m") == 0) {
    alg = MultiLevel_Queues;
    time_slice = atoi(argv[3]);
    max_wait_time = atoi(argv[4]);
    printf("running with multi level queues\n");
  } else {
    fprintf(stderr,
            "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p | -m <time slice> "
            "<max wait time>]\n"
            "    Default : FIFO Scheduler\n"
            "         -r : Round-Robin Scheduler (must also give time slice)\n"
            "         -p : Static Priority Scheduler\n"
            "         -m : Multi-layer Feedback Queues (must also give time "
            "slice and max wait time)\n\n");
    return -1;
  }
  fflush(stdout);

  /* atoi converts string to integer */
  cpu_count = atoi(argv[1]);

  /* Allocate the current[] array and its mutex */
  current = malloc(sizeof(pcb_t*) * cpu_count);
  int i;
  for (i = 0; i < cpu_count; i++) {
    current[i] = NULL;
  }
  assert(current != NULL);
  pthread_mutex_init(&current_mutex, NULL);

  /* Initialize other necessary synch constructs */
  pthread_mutex_init(&ready_mutex, NULL);
  pthread_cond_init(&ready_empty, NULL);

  /* Start the simulator in the library */
  printf("starting simulator\n");
  fflush(stdout);
  start_simulator(cpu_count);

  return 0;
}

/*
 * idle() is called by the simulator when the idle process is scheduled.
 * It blocks until a process is added to the ready queue, and then calls
 * schedule() to select the next process to run on the CPU.
 *
 * THIS FUNCTION IS ALREADY COMPLETED - DO NOT MODIFY
 */
extern void idle(unsigned int cpu_id) {
  pthread_mutex_lock(&ready_mutex);
  while (head == NULL && head1 == NULL && head2 == NULL && head3 == NULL &&
         head4 == NULL) {
    pthread_cond_wait(&ready_empty, &ready_mutex);
  }
  pthread_mutex_unlock(&ready_mutex);
  schedule(cpu_id);
}

/*
 * schedule() is your CPU scheduler. It currently implements basic FIFO
 *scheduling -
 * 1. calls getReadyProcess to select and remove a runnable process from your
 *ready queue
 * 2. updates the current array to show this process (or NULL if there was none)
 *as
 *    running on the given cpu
 * 3. sets this process state to running (unless its the NULL process)
 * 4. calls context_switch to actually start the chosen process on the given cpu
 *    - note if proc==NULL the idle process will be run
 *    - note the final arg of -1 means there is no clock interrupt
 *	context_switch() is prototyped in os-sim.h. Look there for more
 *information.
 *  a basic getReadyProcess() is implemented below, look at the comments for
 *info.
 *
 * TO-DO: handle scheduling with a time-slice when necessary
 *
 * THIS FUNCTION IS PARTIALLY COMPLETED - REQUIRES MODIFICATION
 */
static void schedule(unsigned int cpu_id) {
  pcb_t* proc;
  /*get ready process from a priority queue if StaticPriority scheduling is
  used
  if multi level scheduling is used, get process from highest non-empty queue
  and increment priority of processes that have waited past the time limit */
  if (alg == StaticPriority) {
    proc = getReadyProcessPriority();
  } else if (alg == MultiLevel_Queues) {
    check_limit();
    proc = getReadyProcessMLQ();
  } else {
    proc = getReadyProcess();
  }

  pthread_mutex_lock(&current_mutex);
  current[cpu_id] = proc;
  pthread_mutex_unlock(&current_mutex);

  if (proc != NULL) {
    proc->state = PROCESS_RUNNING;
  }
  context_switch(cpu_id, proc, time_slice);
}

/*
 * preempt() is called when a process is preempted due to its timeslice
 *expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, then call schedule() to select a new runnable process.
 *
 * THIS FUNCTION MUST BE IMPLEMENTED FOR ROUND ROBIN OR PRIORITY SCHEDULING
 */
extern void preempt(unsigned int cpu_id) {
  pthread_mutex_lock(&current_mutex);
  pcb_t* process = current[cpu_id];
  process->state = PROCESS_READY;
  pthread_mutex_unlock(&current_mutex);
  if (alg == MultiLevel_Queues) {
    if (process->static_priority > 1) {
      process->static_priority--;
    }
    addReadyProcessMLQ(process);
  } else {
    addReadyProcess(process);
  }
  schedule(cpu_id);
}
/*
 * yield() is called by the simulator when a process performs an I/O request
 * note this is different than the concept of yield in user-level threads!
 * In this context, yield sets the state of the process to waiting (on I/O),
 * then calls schedule() to select a new process to run on this CPU.
 * args: int - id of CPU process wishing to yield is currently running on.
 *
 * THIS FUNCTION IS ALREADY COMPLETED - DO NOT MODIFY
 */
extern void yield(unsigned int cpu_id) {

  // use lock to ensure thread-safe access to current process
  pthread_mutex_lock(&current_mutex);
  pcb_t* process = current[cpu_id];
  if (alg == MultiLevel_Queues) {
    // increment priority if io block
    if (process->static_priority < 4) {
      process->static_priority++;
    }
  }
  process->state = PROCESS_WAITING;
  pthread_mutex_unlock(&current_mutex);
  schedule(cpu_id);
}

/*
 * terminate() is called by the simulator when a process completes.
 * marks the process as terminated, then calls schedule() to select
 * a new process to run on this CPU.
 * args: int - id of CPU process wishing to terminate is currently running on.
 *
 * THIS FUNCTION IS ALREADY COMPLETED - DO NOT MODIFY
 */
extern void terminate(unsigned int cpu_id) {
  // use lock to ensure thread-safe access to current process

  pthread_mutex_lock(&current_mutex);
  current[cpu_id]->state = PROCESS_TERMINATED;
  pthread_mutex_unlock(&current_mutex);
  schedule(cpu_id);
}

/*
 * wake_up() is called for a new process and when an I/O request completes.
 * The current implementation handles basic FIFO scheduling by simply
 * marking the process as READY, and calling addReadyProcess to put it in the
 * ready queue.  No locks are needed to set the process state as its not
 *possible
 * for anyone else to also access it at the same time as wake_up
 *
 * TO-DO: If the scheduling algorithm is static priority, wake_up() may need
 * to preempt the CPU with the lowest priority process to allow it to
 * execute the process which just woke up.  However, if any CPU is
 * currently running idle, or all of the CPUs are running processes
 * with a higher priority than the one which just woke up, wake_up()
 * should not preempt any CPUs. To preempt a process, use force_preempt().
 * Look in os-sim.h for its prototype and parameters.
 *
 * THIS FUNCTION IS PARTIALLY COMPLETED - REQUIRES MODIFICATION
 */
extern void wake_up(pcb_t* process) {
  if (alg == MultiLevel_Queues) {
    if (process->state == PROCESS_NEW) {

      process->static_priority = 4;
    }
    process->state = PROCESS_READY;
    addReadyProcessMLQ(process);
  } else {
    process->state = PROCESS_READY;
    addReadyProcess(process);
  }
  // Add function for force preempt when using Static Priority protocol
  if (alg == StaticPriority) {
    pthread_mutex_lock(&current_mutex);
    // get the cpu id with the lowest priority process running
    int lowest = process->static_priority;
    int lowest_cpu = -1;
    for (int cur_cpu = 0; cur_cpu < cpu_count; cur_cpu++) {
      // if some cpu is empty, just replace the idle process
      if (current[cur_cpu] == NULL) {
        lowest_cpu = cur_cpu;
        break;
      } else {
        pcb_t* running_process = current[cur_cpu];
        if (running_process->static_priority < lowest) {
          lowest = running_process->static_priority;
          lowest_cpu = cur_cpu;
        }
      }
    }
    pthread_mutex_unlock(&current_mutex);

    // if there are cpus running processes with lower priority,
    // force preempt the lowest one.
    if (lowest_cpu > -1) {
      force_preempt(lowest_cpu);
    }
  }
}

/* The following 2 functions implement a FIFO ready queue of processes */

/*
 * addReadyProcess adds a process to the end of a pseudo linked list (each
 * process
 * struct contains a pointer next that you can use to chain them together)
 * it takes a pointer to a process as an argument and has no return
 */
static void addReadyProcess(pcb_t* proc) {
  // ensure no other process can access ready list while we update it
  pthread_mutex_lock(&ready_mutex);

  // add this process to the end of the ready list
  if (head == NULL) {
    head = proc;
    tail = proc;
    // if list was empty may need to wake up idle process
    pthread_cond_signal(&ready_empty);
  } else {
    tail->next = proc;
    tail = proc;
  }

  // ensure that this proc points to NULL
  proc->next = NULL;

  pthread_mutex_unlock(&ready_mutex);
}

/* if scheduling is multi level, check process priority and add to corresponding
queue */
static void addReadyProcessMLQ(pcb_t* proc) {

  int simulator_time = get_sim_time();
  proc->wait_limit = simulator_time + max_wait_time;
  switch (proc->static_priority) {
    case 1:
      pthread_mutex_lock(&ready_mutex);

      if (head1 == NULL) {
        head1 = proc;
        tail1 = proc;

        pthread_cond_signal(&ready_empty);
      } else {
        tail1->next = proc;
        tail1 = proc;
      }
      proc->next = NULL;

      pthread_mutex_unlock(&ready_mutex);
      break;
    case 2:
      pthread_mutex_lock(&ready_mutex);

      if (head2 == NULL) {
        head2 = proc;
        tail2 = proc;
        pthread_cond_signal(&ready_empty);
      } else {
        tail2->next = proc;
        tail2 = proc;
      }
      proc->next = NULL;
      pthread_mutex_unlock(&ready_mutex);
      break;
    case 3:
      pthread_mutex_lock(&ready_mutex);

      if (head3 == NULL) {
        head3 = proc;
        tail3 = proc;
        pthread_cond_signal(&ready_empty);
      } else {
        tail3->next = proc;
        tail3 = proc;
      }

      proc->next = NULL;
      pthread_mutex_unlock(&ready_mutex);
      break;
    case 4:
      pthread_mutex_lock(&ready_mutex);
      // add this process to the end of the ready list
      if (head4 == NULL) {
        head4 = proc;
        tail4 = proc;
        // if list was empty may need to wake up idle process
        pthread_cond_signal(&ready_empty);
      } else {
        tail4->next = proc;
        tail4 = proc;
      }
      // ensure that this proc points to NULL
      proc->next = NULL;
      pthread_mutex_unlock(&ready_mutex);
      break;
  }
}

/*
 * getReadyProcess removes a process from the front of a pseudo linked list
 *(each process
 * struct contains a pointer next that you can use to chain them together)
 * it takes no arguments and returns the first process in the ready queue, or
 *NULL
 * if the ready queue is empty
 * if multi level queue scheduling is selected, get process from queues in order
 of decreasing priority
 */
static pcb_t* getReadyProcess(void) {
  // ensure no other process can access ready list while we update it

  pthread_mutex_lock(&ready_mutex);

  // if list is empty, unlock and return null
  if (head == NULL) {
    pthread_mutex_unlock(&ready_mutex);
    return NULL;
  }

  // get first process to return and update head to point to next process
  pcb_t* first = head;
  head = first->next;

  // if there was no next process, list is now empty, set tail to NULL
  if (head == NULL) tail = NULL;

  pthread_mutex_unlock(&ready_mutex);
  return first;
}

// Used in Static Priority
static pcb_t* getReadyProcessPriority(void) {
  // ensure no other process can access ready list while we update it
  pthread_mutex_lock(&ready_mutex);

  // if list is empty, unlock and return null
  if (head == NULL) {
    pthread_mutex_unlock(&ready_mutex);
    return NULL;
  }

  int highest_priority = 0;
  pcb_t* current = head;
  pcb_t* returnee_pre;
  pcb_t* returnee;
  int count = 0;
  pcb_t* previous = NULL;

  // get highest priority process to return
  while (current != NULL) {
    if (current->static_priority > highest_priority) {
      highest_priority = current->static_priority;
      returnee = current;
      returnee_pre = previous;
    }
    previous = current;
    current = previous->next;
    count++;
  }

  // link the process before the returnee in the queue and the process after the
  // returnee
  if (returnee_pre != NULL) {
    returnee_pre->next = returnee->next;
  } else {
    head = returnee->next;
  }
  // if tail is returned, update tail to the process in front of it in the queue
  if (returnee->pid == tail->pid) {
    tail = returnee_pre;
  }
  // if there was no next process, list is now empty, set tail to NULL
  if (count == 1) {
    head = NULL;
    tail = NULL;
  }

  pthread_mutex_unlock(&ready_mutex);

  return returnee;
}

static pcb_t* getReadyProcessMLQ(void) {
  // ensure no other process can access ready list while we update it

  pthread_mutex_lock(&ready_mutex);

  // if all 4 queues are empty, unlock and return null
  if (head4 == NULL) {
    if (head3 == NULL) {
      if (head2 == NULL) {
        if (head1 == NULL) {
          pthread_mutex_unlock(&ready_mutex);

          return NULL;
        }
        // get first process to return and update head to point to next process
        pcb_t* first = head1;
        head1 = first->next;
        // if there was no next process, list is now empty, set tail to NULL
        if (head1 == NULL) tail1 = NULL;
        pthread_mutex_unlock(&ready_mutex);
        first->wait_limit = 0;
        return first;
      }
      pcb_t* first = head2;
      head2 = first->next;
      if (head2 == NULL) tail2 = NULL;
      pthread_mutex_unlock(&ready_mutex);
      first->wait_limit = 0;
      return first;
    }
    pcb_t* first = head3;
    head3 = first->next;
    if (head3 == NULL) tail3 = NULL;
    pthread_mutex_unlock(&ready_mutex);
    first->wait_limit = 0;
    return first;
  }
  pcb_t* first = head4;
  head4 = first->next;
  if (head4 == NULL) tail4 = NULL;
  pthread_mutex_unlock(&ready_mutex);
  first->wait_limit = 0;
  return first;
}
