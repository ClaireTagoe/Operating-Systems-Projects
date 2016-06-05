/*
 * os-sim.c
 * Multithreaded OS Simulation - original file from project 4 at
 * http://www.cc.gatech.edu/~rama/CS2200-External
 *
 * The simulator internals.
 *
 * Last modified 2/23/2014 by Sherri Goings
 *
 * DO NOT MODIFY THIS FILE
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "os-sim.h"
#include "process.h"
#include "student.h"

typedef enum {
  CPU_IDLE = 0,
  CPU_RUNNING,
  CPU_PREEMPT,
  CPU_YIELD,
  CPU_TERMINATE
} simulator_cpu_state_t;

typedef struct {
  pcb_t *current;
  simulator_cpu_state_t state;
  pthread_cond_t wakeup;
  int preemption_timer;
} simulator_cpu_data_t;

/* The I/O queue is a simple, FIFO queue using a linked list */
typedef struct _io_request {
  pcb_t *pcb;
  unsigned int execution_time;
  struct _io_request *next;
} io_request;

static io_request *io_queue_head = NULL, *io_queue_tail = NULL;
static simulator_cpu_data_t *simulator_cpu_data;
static pthread_t *cpu_thread;
static pthread_mutex_t simulator_mutex;
static pthread_cond_t thread_yielded;
static unsigned int simulator_time = 0;
static unsigned int processes_terminated = 0;
static unsigned int cpu_count;
static unsigned int ready_counter = 0, running_counter = 0, waiting_counter = 0;
static unsigned int context_switches = 0;

static void simulator_supervisor_thread(void);
static void simulator_cpu_thread(unsigned int cpu_id);

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);

static void print_gantt_header(void);
static void print_gantt_line(void);
static void print_final_stats(void);

static void simulate_cpus(void);
static void simulate_process(unsigned int cpu_id, pcb_t *pcb);
static void submit_io_request(pcb_t *pcb, unsigned int execution_time);
static void simulate_io(void);
static void simulate_creat(void);

static void *simulator_cpu_thread_func(void *data);

/*
 * IRWL - An "Inverted" Readers-Writers Lock
 *
 * Unlike a traditional readers-writers lock, this lock allows infinitely
 * many writers or one reader.
 *
 * Its purpose is to protect the state variable of the PCB structures, which
 * is accessed both by the student's code and by print_gantt_line().  We
 * could use a simple mutex, and lock it while calling any student's code,
 * but then the student's code wouldn't get tested for thread-safeness.
 * So we will intentionally let multiple pieces of the student's code run
 * simultaneously.
 *
 * For the student_lock, the IRWL_WRITER should always be locked while
 * student code is executing on a CPU thread.  The IRWL_READER should always be
 * locked whenever non-constant data in a PCB is used by the library.
 */
typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t no_writers;
  int writers;
} irwl;

#define IRWL_INIT(i)                        \
  pthread_mutex_init(&(i).mutex, NULL);     \
  pthread_cond_init(&(i).no_writers, NULL); \
  (i).writers = 0;

#define IRWL_READER_LOCK(i)                         \
  pthread_mutex_lock(&(i).mutex);                   \
  while ((i).writers > 0) {                         \
    pthread_cond_wait(&(i).no_writers, &(i).mutex); \
  }

#define IRWL_READER_UNLOCK(i) pthread_mutex_unlock(&(i).mutex);

#define IRWL_WRITER_LOCK(i)       \
  pthread_mutex_lock(&(i).mutex); \
  (i).writers++;                  \
  pthread_mutex_unlock(&(i).mutex);

#define IRWL_WRITER_UNLOCK(i)             \
  pthread_mutex_lock(&(i).mutex);         \
  (i).writers--;                          \
  if ((i).writers == 0) {                 \
    pthread_cond_signal(&(i).no_writers); \
  }                                       \
  pthread_mutex_unlock(&(i).mutex);

static irwl student_lock;

/* The big initialization function */
extern void start_simulator(unsigned int new_cpu_count) {
  int n;

  /* Make sure the # of CPUs is reasonable */
  cpu_count = new_cpu_count;
  if (cpu_count < 1 || cpu_count > 16) {
    fprintf(stderr, "CPU Count must be an integer from 1 to 16!\n\n");
    exit(-1);
  }

  /* Allocate arrays */
  cpu_thread = malloc(sizeof(pthread_t) * cpu_count);
  assert(cpu_thread != NULL);
  simulator_cpu_data = malloc(sizeof(simulator_cpu_data_t) * cpu_count);
  assert(simulator_cpu_data != NULL);

  /* Initialize mutexes and condition variables */
  pthread_mutex_init(&simulator_mutex, NULL);
  pthread_cond_init(&thread_yielded, NULL);
  simulator_time = 0;
  for (n = 0; n < cpu_count; n++) {
    simulator_cpu_data[n].current = NULL;
    simulator_cpu_data[n].state = CPU_IDLE;
    simulator_cpu_data[n].preemption_timer = -1;
    pthread_cond_init(&simulator_cpu_data[n].wakeup, NULL);
  }

  IRWL_INIT(student_lock)

  /* Start CPU threads */
  for (n = 0; n < cpu_count; n++)
    pthread_create(&cpu_thread[n], NULL, simulator_cpu_thread_func,
                   (void *)(long)n);

  /* Start supervisor thread */
  simulator_supervisor_thread();
}

/*
 * This is the loop for the supervisor thread.  It waits for 100ms, then
 * simulates one interval of time.
 */
static void simulator_supervisor_thread(void) {
  print_gantt_header();

  /* Loop, performing execution every 100ms.  At each execution, we will
     display a line in the Gantt chart and check for pending I/O requests */
  while (1) {
    pthread_mutex_lock(&simulator_mutex);

    /* Exit when all processes terminate */
    if (processes_terminated >= PROCESS_COUNT) {
      print_final_stats();
      exit(0);
    }

    print_gantt_line();
    simulate_cpus();
    simulate_io();
    simulate_creat();
    simulator_time++;
    pthread_mutex_unlock(&simulator_mutex);

    mt_safe_usleep(1);
  }
}

/*
 * This is the loop for the CPU threads.  The general idea:
 *
 *   1) Each CPU thread has a state variable.  While the library is using the
 *      CPU thread to simulate a process, this variable is set to CPU_RUNNING.
 *
 *   2) To "simulate" a process, we simply block on a condition variable.
 *      Each CPU thread has a dedicated condition variable.
 *
 *   3) For simplicity, the supervisor thread actually does all of the work.
 *      This makes synchronization in the simulator much easier, since all
 *      the real work is done by a single thread.  So, when the supervisor
 *      wants to dispatch an event to a CPU thread, it needs to unblock the
 *      CPU thread.  It does this by setting the CPU thread's state variable
 *      to inform the CPU thread of the event, then it signals the condition
 *      variable.
 *
 *   4) Once the CPU thread unblocks, it calls the students event handler,
 *      then goes back to step 1.
 *
 * There is one special case: idle.  Idle is simulated by the student's code,
 * not the library's.  So we simply set the state variable to CPU_IDLE, and
 * call the student's code.
 */
static void simulator_cpu_thread(unsigned int cpu_id) {
  simulator_cpu_state_t state;

  while (1) {
    pthread_mutex_lock(&simulator_mutex);
    if (simulator_cpu_data[cpu_id].current == NULL) {
      /* the idle process was selected */
      simulator_cpu_data[cpu_id].state = CPU_IDLE;
    } else {
      /* a process was scheduled */
      simulator_cpu_data[cpu_id].state = CPU_RUNNING;

      while (simulator_cpu_data[cpu_id].state == CPU_RUNNING)
        pthread_cond_wait(&simulator_cpu_data[cpu_id].wakeup, &simulator_mutex);
    }
    state = simulator_cpu_data[cpu_id].state;
    pthread_mutex_unlock(&simulator_mutex);

    /* Call student's code */
    switch (state) {
      case CPU_IDLE:
        /*
         * We can't lock the student_lock for idle(); otherwise we can't
         * print statistics while any CPU is idling.
         */
        idle(cpu_id);
        break;

      case CPU_PREEMPT:
        IRWL_WRITER_LOCK(student_lock)
        preempt(cpu_id);
        IRWL_WRITER_UNLOCK(student_lock)
        break;

      case CPU_YIELD:
        IRWL_WRITER_LOCK(student_lock)
        yield(cpu_id);
        IRWL_WRITER_UNLOCK(student_lock)
        break;

      case CPU_TERMINATE:
        processes_terminated++;
        IRWL_WRITER_LOCK(student_lock)
        terminate(cpu_id);
        IRWL_WRITER_UNLOCK(student_lock)
        break;

      case CPU_RUNNING:
        /* This should never happen!!! */
        break;
    }
  }
}

/*
 * print_gantt_header() and print_gantt_line() are helper functions to display
 * the Gantt Chart.
 */
static void print_gantt_header(void) {
  int n;

  printf("Time  Ru Re Wa     ");
  for (n = 0; n < cpu_count; n++) printf(" CPU %d   ", n);
  printf(
      "     < I/O Queue <\n"
      "===== == == ==     ");
  for (n = 0; n < cpu_count; n++) printf(" ========");
  printf("     =============\n");
}

static void print_gantt_line(void) {
  io_request *r;
  unsigned int current_ready = 0, current_running = 0, current_waiting = 0;
  int n;

  /*
   * Update number of processes in each state.
   */
  IRWL_READER_LOCK(student_lock)
  for (n = 0; n < PROCESS_COUNT; n++) {
    switch (processes[n].state) {
      case PROCESS_READY:
        current_ready++;
        ready_counter++;
        break;

      case PROCESS_RUNNING:
        current_running++;
        running_counter++;
        break;

      case PROCESS_WAITING:
        current_waiting++;
        waiting_counter++;
        break;

      default:
        break;
    }
  }
  IRWL_READER_UNLOCK(student_lock)

  /* Print time */
  printf("%-5.1f %-2d %-2d %-2d     ", (float)simulator_time / 10.0,
         current_running, current_ready, current_waiting);

  /* Print running processes */
  for (n = 0; n < cpu_count; n++) {
    if (simulator_cpu_data[n].current != NULL)
      printf(" %-8s", simulator_cpu_data[n].current->name);
    else
      printf(" (IDLE)  ");
  }

  /* Print I/O requests */
  printf("     <");
  r = io_queue_head;
  while (r != NULL) {
    printf(" %s", r->pcb->name);
    r = r->next;
  }
  printf(" <\n");
}

static void print_final_stats(void) {
  printf("\n\n");
  printf("# of Context Switches: %u\n", context_switches);
  printf("Total execution time: %.1f s\n", (float)simulator_time / 10.0);
  printf("Total time spent in READY state: %.1f s\n",
         (float)ready_counter / 10.0);
}

/*
 * context_switch() and force_preempt() are the two functions available to
 * student's code.
 */
extern void context_switch(unsigned int cpu_id, pcb_t *pcb,
                           int preemption_time) {
  assert(cpu_id < cpu_count);
  assert(pcb == NULL ||
         (pcb >= processes && pcb <= processes + PROCESS_COUNT - 1));

  context_switches++;

  IRWL_WRITER_UNLOCK(student_lock);
  pthread_mutex_lock(&simulator_mutex);
  simulator_cpu_data[cpu_id].current = pcb;
  simulator_cpu_data[cpu_id].preemption_timer = preemption_time;
  pthread_cond_signal(&thread_yielded);
  pthread_mutex_unlock(&simulator_mutex);
  IRWL_WRITER_LOCK(student_lock);
}

extern void force_preempt(unsigned int cpu_id) {
  // printf("UUUUUUUUUUU-- force_preempt: %d\n", cpu_id);
  // fflush(stdout);
  assert(cpu_id < cpu_count);

  IRWL_WRITER_UNLOCK(student_lock);
  pthread_mutex_lock(&simulator_mutex);

  /*
   * It is possible that the student's code calls force_preempt() at the
   * same time the process was already going to yield or terminate.  We
   * check for that case by only preempting if the CPU is set to CPU_RUNNING.
   */
  if (simulator_cpu_data[cpu_id].state == CPU_RUNNING) {
    simulator_cpu_data[cpu_id].state = CPU_PREEMPT;
    pthread_cond_signal(&simulator_cpu_data[cpu_id].wakeup);
    // wait to make sure thread finishes preempt and context switch
    pthread_cond_wait(&thread_yielded, &simulator_mutex);
  }

  pthread_mutex_unlock(&simulator_mutex);
  IRWL_WRITER_LOCK(student_lock);
}
// return simulation time
extern int get_sim_time() { return simulator_time; }

/*
 * The functions below are used by the supervisor thread to simulate the OS.
 *
 * simulate_cpus() / simulate_process() simulate the processes on each CPU
 *   and signal the appropriate CPU thread if an event occurs.
 *
 * submit_io_request() inserts a PCB into tail of the I/O queue.
 *
 * simulate_io() simulates the I/O request at the head of the I/O queue and
 *   calls wake_up() upon completion.
 *
 * simulate_creat() simulates initial process creation by calling the
 *   student's wake_up().
 */

static void simulate_cpus(void) {
  int n;

  for (n = 0; n < cpu_count; n++) {
    if (simulator_cpu_data[n].current != NULL)
      simulate_process(n, simulator_cpu_data[n].current);
  }
}

static void simulate_process(unsigned int cpu_id, pcb_t *pcb) {
  /*
   * The "program counter" is really just a pointer to the current position
   * in the operations array
   */
  op_t *pc = (op_t *)pcb->pc;

  switch (pc->type) {
    case OP_CPU:
      /* Scheduling a running process ... good ... */

      /* Check to see if the CPU burst has completed */
      if (pc->time > 0) {
        /* Simulate running the process */
        pc->time--;

        /* Simulate the preemption timer */
        simulator_cpu_data[cpu_id].preemption_timer--;
        if (simulator_cpu_data[cpu_id].preemption_timer == 0) {
          /* The timer has expired; preempt the running process */
          simulator_cpu_data[cpu_id].state = CPU_PREEMPT;
          pthread_cond_signal(&simulator_cpu_data[cpu_id].wakeup);
          // wait to make sure thread finishes preempt and context switch
          pthread_cond_wait(&thread_yielded, &simulator_mutex);
        }
      } else {
        /* Move to the next operation */
        pcb->pc = ((op_t *)(pcb->pc)) + 1;
        pc++;

        switch (pc->type) {
          case OP_IO:
            /* Put a request in the I/O FIFO queue */
            submit_io_request(pcb, pc->time);

            /* Generate a yield() call on the appropriate CPU */
            simulator_cpu_data[cpu_id].state = CPU_YIELD;
            pthread_cond_signal(&simulator_cpu_data[cpu_id].wakeup);
            // wait to make sure thread finishes yield and context switch
            pthread_cond_wait(&thread_yielded, &simulator_mutex);
            break;

          case OP_TERMINATE:
            /* Generate a terminate() call on the appropriate CPU */
            simulator_cpu_data[cpu_id].state = CPU_TERMINATE;
            pthread_cond_signal(&simulator_cpu_data[cpu_id].wakeup);
            // wait to make sure thread finishes terminate and context switch
            pthread_cond_wait(&thread_yielded, &simulator_mutex);
            break;

          case OP_CPU:
            break;
        }
      }
      break;

    case OP_IO:
      /* Scheduling a process that's blocked on I/O */
      printf("Scheduled a process that's blocked on I/0! PID: %d\n", pcb->pid);
      break;

    case OP_TERMINATE:
      /* Scheduling a process that's terminated */
      printf("Scheduled a terminated process! PID: %d\n", pcb->pid);
      break;
  }
}

static void submit_io_request(pcb_t *pcb, unsigned int execution_time) {
  io_request *r;

  /* Build I/O Request */
  r = malloc(sizeof(io_request));
  assert(r != NULL);
  r->pcb = pcb;
  r->execution_time = execution_time;
  r->next = NULL;

  /* Add request to end of queue */
  if (io_queue_tail != NULL) {
    io_queue_tail->next = r;
    io_queue_tail = r;
  } else {
    io_queue_head = r;
    io_queue_tail = r;
  }
}

static void simulate_io(void) {
  if (io_queue_head == NULL) return; /* There are no I/O requests */

  if (io_queue_head->execution_time-- <= 0) {
    io_request *completed = io_queue_head;
    pcb_t *pcb;

    /* Move the programs "PC" to the next "instruction" */
    completed->pcb->pc = ((op_t *)completed->pcb->pc) + 1;

    /*
     * Remove the I/O request from the queue before calling the student's
     * code.  We must do this, because once we release the simulator_mutex,
     * the I/O queue may have changed.
     */
    pcb = completed->pcb;
    io_queue_head = completed->next;
    if (io_queue_head == NULL) io_queue_tail = NULL;
    free(completed);

    /* Call the student's wake_up() handler */
    pthread_mutex_unlock(&simulator_mutex);
    IRWL_WRITER_LOCK(student_lock);
    wake_up(pcb);
    IRWL_WRITER_UNLOCK(student_lock);
    pthread_mutex_lock(&simulator_mutex);
  }
}

static void simulate_creat(void) {
  static int processes_created = 0;

  if ((simulator_time % 10) == 0 && processes_created < PROCESS_COUNT) {
    /* Call student's wake_up() handler */
    pthread_mutex_unlock(&simulator_mutex);
    IRWL_WRITER_LOCK(student_lock);
    wake_up(&processes[processes_created]);
    IRWL_WRITER_UNLOCK(student_lock);
    pthread_mutex_lock(&simulator_mutex);

    processes_created++;
  }
}

/* Cheap hack -- passing an int through a void pointer */
static void *simulator_cpu_thread_func(void *data) {
  simulator_cpu_thread((int)(long)data);
  return NULL;
}

/* mt_safe_usleep() emulates the usleep() function, but is thread-safe */
extern void mt_safe_usleep(unsigned long usec) {
  struct timespec ts;
  ts.tv_sec = usec / 1000000;
  ts.tv_nsec = (usec % 1000000) * 1000;

  while (nanosleep(&ts, &ts) != 0)
    ;
}

// check if process wait time has exceeded limit and increment its priority
// if it has
extern void check_limit() {
  for (int n = 0; n < PROCESS_COUNT; n++) {
    if (processes[n].state == PROCESS_READY) {
      if (processes[n].wait_limit >= get_sim_time()) {
        if (processes[n].static_priority < 4) processes[n].static_priority++;
      }
    }
  }
}
