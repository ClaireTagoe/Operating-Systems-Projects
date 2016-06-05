#ifndef _STASTICS_H_
#define _STASTICS_H_

/*******************************************************************************
 * Display the stastics gathered from executing the simulator.
 */
void display_statistics();

/*******************************************************************************
 * Compute the Average Mean Access Time with unavoidable page faults.
 */
double compute_emat_all();

/*******************************************************************************
 * Compute the Average Mean Access Time without unavoidable page faults.
 */
double compute_emat_unforced();

/*******************************************************************************
 * Number of pagefaults generated during execution.
 */
extern unsigned long int count_pagefaults;

/*******************************************************************************
 * Number of TLB hits generated during execution.
 */
extern unsigned long int count_tlbhits;

/*******************************************************************************
 * Raw number of stores executed.
 */
extern unsigned long int count_writes;

/*******************************************************************************
 * Raw number of loads executed.
 */
extern unsigned long int count_reads;

/*******************************************************************************
 * Raw number of loads executed.
 */
extern unsigned long int count_diskaccesses;

#endif/*_STASTICS_H_*/
