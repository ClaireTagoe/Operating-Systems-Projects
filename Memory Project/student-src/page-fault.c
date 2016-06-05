#include <stdio.h>
#include <assert.h>

#include "types.h"
#include "process.h"
#include "global.h"
#include "swapfile.h"
#include "statistics.h"


/*******************************************************************************
 * Page fault handler. When the CPU encounters an invalid address mapping in a
 * process' page table, it invokes the CPU via this handler. The OS then
 * allocates a physical frame for the requested page (either by using a free
 * frame or evicting one), changes the process' page table to reflect the
 * mapping and then restarts the interrupted process.
 *
 * @param vpn The virtual page number requested from the current process.
 * @param write If the CPU is writing to the page, this is 1. Otherwise, it's 0.
 * @return The physical frame the OS has mapped to the virtual page.
 */
pfn_t pagefault_handler(vpn_t request_vpn, int write) {
  pfn_t victim_pfn;
  vpn_t victim_vpn;
  pcb_t *victim_pcb;

  /* Sanity Check */
  assert(current_pagetable != NULL);

  /* Find a free frame */
  victim_pfn = get_free_frame();
  assert(victim_pfn < CPU_NUM_FRAMES); /* make sure the victim_pfn is valid */

  /* Use the reverse lookup table to find the victim. */
  victim_vpn = rlt[victim_pfn].vpn;
  victim_pcb = rlt[victim_pfn].pcb;

  /*
   * FIX ME : Problem 4
   * If victim page is occupied - if it is not the pcb will be NULL:
   *
   * 1) If it's dirty, save it to disk with page_to_disk()
   * 2) Invalidate the page's entry in the victim's page table.
   * 3) Clear the victim page's TLB entry using the function tlb_clearone().

   */

   if (victim_pcb) {
     if (victim_pcb->pagetable[victim_vpn].dirty) {
       printf("victim pid: %d\n", victim_pcb->pid);
       fflush(stdout);
       page_to_disk(victim_pfn, victim_vpn, victim_pcb->pid);
     }
     victim_pcb->pagetable[victim_vpn].valid = 0;
     victim_pcb->pagetable[victim_vpn].dirty = 0;
     if (victim_pcb->pid == current->pid){
       tlb_clearone(victim_vpn);
     }
   }

  printf("PAGE FAULT (VPN %u), evicting (PFN %u VPN %u)\n", request_vpn,
      victim_pfn, victim_vpn);


  /* FIX ME */
  /* Update the reverse lookup table to replace the victim entry info with this
   * process' info instead (pcb and vpn)
   * Update the current process' page table (pfn and valid)
   */
   rlt[victim_pfn].vpn = request_vpn;
   rlt[victim_pfn].pcb = current;
   current_pagetable[request_vpn].pfn = victim_pfn;

  /*
   * Retreive the page from disk. Note that is really a lie: we save pages in
   * memory (since doing file I/O for this simulation would be annoying and
   * wouldn't add that much to the learning). Also, if the page technically
   * does't exist yet (i.e., the page has never been accessed yet, we return a
   * blank page. Real systems would check for invalid pages and possibly read
   * stuff like code pages from disk. For purposes of this simulation, we won't
   * worry about that. =)
   */
  page_from_disk(victim_pfn, request_vpn, current->pid);

  return victim_pfn;
}
