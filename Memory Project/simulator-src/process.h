#ifndef _PROCESS_H_
#define _PROCESS_H_

#include "types.h"
#include "pagetable.h"

/*******************************************************************************
 * Process control block. Contains the information on a running process.
 */
typedef struct {
   int pid;          /* Process identifier */
   char name[20];    /* User-specified name of the process */
   pte_t *pagetable; /* Process page table */
} pcb_t;

/*******************************************************************************
 * Entry in the reverse lookup table.
 */
typedef struct {
   pcb_t *pcb; /* process control block of the owning process*/
   vpn_t  vpn; /* virtual page number for the owning process this frame holds */
} rlt_t;

/*******************************************************************************
 * Pointer to the task currently running on the CPU.
 */
extern pcb_t* current;

/*******************************************************************************
 * The Reverse Lookup Table.  Maps each physical frame to its owning process.
 * Useful for when we have to evict pages and need to update the owning process'
 * page table.
 */
extern rlt_t* rlt;

/*******************************************************************************
 * Maximum number of jobs to start when reading the references file. Actually,
 * just specifies the maximum PID that will be allowed to fork, load or store.
 */
extern unsigned max_jobs;

/*******************************************************************************
 * Creates a new process & allocates its page table.
 *
 * @param pid The process identifier of the new process.
 * @param name The user-specified name of the new process.
 */
void proc_fork(int pid, const char *name);

/*******************************************************************************
 * Retreive the name of a process associated with the given PID.
 *
 * @param pid The process identifer.
 */
const char *proc_getname(int pid);

/*******************************************************************************
 * Switch the currently running task, updating the current pointer, CPU page
 * register and clearing the TLB.
 *
 * @param pid The process to which to switch
 */
void proc_switch(int pid);

/*******************************************************************************
 * Initialize storage needed by the process management system.
 */
void proc_init(void);

/*******************************************************************************
 * Free storage used by the process management system.
 */
void proc_free(void);

#endif/*_PROCESS_H_*/
