#include <stdlib.h>

#include "types.h"
#include "pagetable.h"
#include "global.h"
#include "process.h"

/*******************************************************************************
 * Finds a free physical frame. If none are available, uses a clock sweep
 * algorithm to find a used frame for eviction.
 *
 * @return The physical frame number of a free (or evictable) frame.
 */
pfn_t get_free_frame(void) {
   int i;

   /* See if there are any free frames */
   for (i = 0; i < CPU_NUM_FRAMES; i++)
      if (rlt[i].pcb == NULL)
         return i;

   /* FIX ME : Bonus */
   /* 
   * IMPLEMENT A CLOCK SWEEP ALGORITHM HERE 
   * Note you can access whether a page is used or not with the following
   * rlt[i].pcb->pagetable[rlt[i].vpn].used
   * but it's a little ridiculous, so you should probably add a used field 
   * to the rlt itself instead and update it appropriately!
   */

   /* If all else fails, return a random frame */
   return rand() % CPU_NUM_FRAMES;
}
