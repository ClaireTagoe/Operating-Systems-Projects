#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "tlb.h"
#include "useful.h"

/*******************************************************************************
 * The TLB is an array of TLB Entries.
 */
tlbe_t* tlb;

/*******************************************************************************
 * Clears ALL entries in the TLB. Used on context switch so that virtual
 * mappings are valid.
 */
void tlb_clearall(void) {
   int i;

   for (i = 0; i < tlb_size; i++) {
      tlb[i].valid = 0;
      tlb[i].dirty = 0;
      tlb[i].used = 0;
   }
}

/*******************************************************************************
 * Clears a single TLB entry, specified by the virtual page number. Used when
 * pages are swapped out so all virtual mappings remain valid.
 *
 * @param vpn The virtual page number of the TLB entry to invalidate.
 */
void tlb_clearone(vpn_t vpn) {
   int i;

   for (i = 0; i < tlb_size; i++) {
      if (tlb[i].vpn == vpn) {
         tlb[i].valid = 0;
         tlb[i].dirty = 0;
         tlb[i].used = 0;
      }
   }
}

/*******************************************************************************
 * Initializes the memory for representing the TLB.
 */
void tlb_init(void) {
   if ((tlb = calloc(tlb_size, sizeof(tlbe_t))) == NULL) {
      PERROR("calloc");
      exit(EXIT_FAILURE);
   }
}

/*******************************************************************************
 * Frees the memory used for representing the TLB.
 */
void tlb_free(void) {
   free(tlb);
}
