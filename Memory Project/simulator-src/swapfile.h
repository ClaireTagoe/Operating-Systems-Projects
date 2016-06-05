#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include "types.h"

/*******************************************************************************
 * Page fault handler. When the CPU encounters an invalid address mapping in a
 * process' page table, it invokes the CPU via this handler. The OS then
 * allocates a physical frame for the requested page (either by using a free
 * frame or evicting one), changes the process' page table to reflect the
 * mapping and then restarts the interrupted process.
 *
 * @param vpn The virtual page number requested.
 * @param write If the CPU is writing to the page, this is 1. Otherwise, it's 0.
 * @return The physical frame the OS has mapped to the virtual page.
 */
pfn_t pagefault_handler(vpn_t request_vpn, int write);

/*******************************************************************************
 * Save the given physical frame to disk with the given virtual page
 * information.
 *
 * @param pfn The physical frame to save.
 * @param vpn The virtual page number this frame holds.
 * @param pid The process identifier of the process that owns this frame.
 */
void page_to_disk(pfn_t pfn, vpn_t vpn, int pid);

/*******************************************************************************
 * Restores the given virtual page to the given physical frame.
 *
 * @param pfn The physical frame to restore the data to.
 * @param vpn The virtual page number of the page to restore.
 * @param pid The process identifer of the page's owner process.
 */
void page_from_disk(pfn_t pfn, vpn_t vpn, int pid);

/*******************************************************************************
 * Free the linked list maintained by the paging system.
 */
void swapfile_free(void);

#endif/*_SWAPFILE_H_*/
