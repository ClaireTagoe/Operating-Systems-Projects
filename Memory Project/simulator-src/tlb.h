#ifndef _TLB_H_
#define _TLB_H_

#include "types.h"

/*******************************************************************************
 * TLB entry.
 */
typedef struct {
  vpn_t vpn;     /* Virtual page number */
  pfn_t pfn;     /* Physical frame number */
  uint8_t valid; /* Valid 'bit' */
  uint8_t dirty; /* Dirty 'bit' */
  uint8_t used;  /* Used (aka recently accessed) 'bit' */
} tlbe_t;

/*******************************************************************************
 * Make the TLB an extern variable, so that other files can access it.
 */
extern tlbe_t* tlb;

/*******************************************************************************
 * Looks up an address in the TLB. If no entry is found, attempts to access the
 * current page table via pagetable_lookup().
 *
 * @param vpn The virtual page number to lookup.
 * @param write If the access is a write, this is 1. Otherwise, it is 0.
 * @return The physical frame number of the page we are accessing.
 */
pfn_t tlb_lookup(vpn_t vpn, int write);

/*******************************************************************************
 * Clears ALL entries in the TLB. Used on context switch so that virtual
 * mappings are valid.
 */
void tlb_clearall(void);

/*******************************************************************************
 * Clears a single TLB entry, specified by the virtual page number. Used when
 * pages are swapped out so all virtual mappings remain valid.
 *
 * @param vpn The virtual page number of the TLB entry to invalidate.
 */
void tlb_clearone(vpn_t vpn);

/*******************************************************************************
 * Initializes the memory for representing the TLB.
 */
void tlb_init(void);

/*******************************************************************************
 * Frees the memory used for representing the TLB.
 */
void tlb_free(void);

#endif/*_TLB_H_*/
