#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "global.h"
#include "useful.h"
#include "tlb.h"

#include "process.h"

/*******************************************************************************
 * The maximum number of jobs that can be created by the main.
 */
unsigned max_jobs = 8;

/*******************************************************************************
 * An array of all the processes.
 */
pcb_t* proc_list;

/*******************************************************************************
 * Pointer to the current process.
 */
pcb_t* current;

/*******************************************************************************
 * The reverse lookp table.
 */
rlt_t* rlt;

/*******************************************************************************
 * Creates a new process & allocates its page table.
 *
 * @param pid The process identifier of the new process.
 * @param name The user-specified name of the new process.
 */
void proc_fork(int pid, const char *name) {
   pte_t *newtbl;

   /* Sanity check */
   assert(pid < max_jobs);

   /* Allocate a new page table */
   if ((newtbl = calloc(CPU_NUM_PTE, sizeof(pte_t))) == NULL) {
      PERROR("calloc");
      exit(EXIT_FAILURE);
   }

   /* Fill in PCB information */
   proc_list[pid].pid = pid;
   proc_list[pid].pagetable = newtbl;
   strncpy(proc_list[pid].name, name, sizeof(proc_list[pid].name));
}

/*******************************************************************************
 * Retreive the name of a process associated with the given PID.
 *
 * @param pid The process identifer.
 */
const char *proc_getname(int pid) {
   assert(pid < max_jobs); 
   return proc_list[pid].name;
}

/*******************************************************************************
 * Switch the currently running task, updating the current pointer, CPU page
 * register and clearing the TLB.
 *
 * @param pid The process to which to switch (I'm a poet).
 */
void proc_switch(int pid) {
   assert(pid < max_jobs);
   assert(proc_list[pid].pagetable != NULL);
   current = &proc_list[pid];
   current_pagetable = current->pagetable;
   tlb_clearall();
}


/*******************************************************************************
 * Initializes memory used by the for process management. 
 * This includes the process list and the reverse lookup table.
 */
void proc_init(void) {
   if ((proc_list = calloc(max_jobs, sizeof(pcb_t))) == NULL ||
       (rlt = calloc(CPU_NUM_FRAMES, sizeof(rlt_t))) == NULL) {
      PERROR("calloc");
      exit(EXIT_FAILURE);
   }
}

/*******************************************************************************
 * Cleans up memory used by the OS. This includes the process list and the
 * reverse lookup table, as well as any allocated page tables.
 */
void proc_free(void) {
   int i;

   /* Loop through all PIDs and free page tables */
   for (i = 0; i < max_jobs; i++) {
      if (proc_list[i].pagetable != NULL)
         free(proc_list[i].pagetable);
   }

   /* Free PCBs - a complete environmental disaster! */
   free(proc_list);
   free(rlt);
}
