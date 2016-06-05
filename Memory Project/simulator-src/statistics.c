#include <stdio.h>

#include "statistics.h"

/*******************************************************************************
 * Number of pagefaults generated during execution.
 */
unsigned long int count_pagefaults = 0;

/*******************************************************************************
 * Number of TLB hits generated during execution.
 */
unsigned long int count_tlbhits = 0;

/*******************************************************************************
 * Raw number of stores executed.
 */
unsigned long int count_writes = 0;

/*******************************************************************************
 * Raw number of loads executed.
 */
unsigned long int count_reads = 0;

/*******************************************************************************
 * Raw number of disk accesses.
 */
unsigned long int count_diskaccesses = 0;

/*******************************************************************************
 * Display the stastics.
 */
void display_statistics() {
  printf("\n-----------------------\n"
  "Statistics:\n"
  "-----------------------\n"
  "Reads:         %10lu\n"
  "Writes:        %10lu\n"
  "TLB Hits:      %10lu\n"
  "Page Faults:   %10lu\n"
  "Disk Accesess: %10lu\n"
  "EMAT all:               %.1f ns\n"
  "EMAT unforced:          %.1f ns\n\n",
  count_reads, count_writes, count_tlbhits, count_pagefaults,
 count_diskaccesses,  compute_emat_all(), compute_emat_unforced());
 fflush(stdout);
}
