/*
 * student.h
 * Multithreaded OS Simulation - original file from project 4 at
 * http://www.cc.gatech.edu/~rama/CS2200-External
 *
 * Last modified 2/23/2014 by Sherri Goings
 *
 * YOU WILL NOT NEED TO MODIFY THIS FILE
 */

#ifndef __STUDENT_H__
#define __STUDENT_H__

#include "os-sim.h"

/* Functions called from simulator - comments in student.c */
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t* process);

/* Functions available to use in student.c to manipulate ready queue */
static void addReadyProcess(pcb_t* proc);
static void addReadyProcessMLQ(pcb_t* proc);
static pcb_t* getReadyProcess(void);
static pcb_t* getReadyProcessPriority(void);
static pcb_t* getReadyProcessMLQ(void);

/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t** current;
static pthread_mutex_t current_mutex;

// head and tail of ready queue
static pcb_t* head = NULL;
static pcb_t* tail = NULL;

static pcb_t* head1 = NULL;
static pcb_t* tail1 = NULL;
static pcb_t* head2 = NULL;
static pcb_t* tail2 = NULL;
static pcb_t* head3 = NULL;
static pcb_t* tail3 = NULL;
static pcb_t* head4 = NULL;
static pcb_t* tail4 = NULL;

// mutex to protect ready queue
static pthread_mutex_t ready_mutex;

// cond var for idle() to sleep on until a process is available on the ready
// queue
static pthread_cond_t ready_empty;

#endif /* __STUDENT_H__ */
