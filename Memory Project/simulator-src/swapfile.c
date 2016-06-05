#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "global.h"
#include "useful.h"
#include "statistics.h"

/* let us modify the memory table -- slightly bad style, but it works */
extern uint8_t* memory;

/*******************************************************************************
 * Simulated swap file. Really just a linked list of saved pages.
 */
struct swap_page {
   int pid;                /* Process identifier */
   vpn_t vpn;              /* Virtual page number */
   unsigned char *data;    /* Saved page data */
   struct swap_page *next; /* Pointer to next saved page */
} *swapfile;              /* Head of our global swapfile list */

/*******************************************************************************
 * Save the given physical frame to disk with the given virtual page
 * information.
 *
 * @param pfn The physical frame to save.
 * @param vpn The virtual page number this frame holds.
 * @param pid The process identifier of the process that owns this frame.
 */
void page_to_disk(pfn_t pfn, vpn_t vpn, int pid) {
   struct swap_page *newp;
   count_diskaccesses++;

   /* Allocate a new swap entry and data space for this frame */
   if ((newp = calloc(1, sizeof(struct swap_page))) == NULL ||
       (newp->data = calloc(page_size, 1)) == NULL) {
      PERROR("calloc");
      exit(EXIT_FAILURE);
   }

   /* Fill in appropriate data */
   newp->pid = pid;
   newp->vpn = vpn;
   newp->next = swapfile;
   memcpy(newp->data, &memory[pfn * page_size], page_size);

   /* Add to the head of the swapfile (since I like O(1) operations). */
   swapfile = newp;
}

/*******************************************************************************
 * Restores the given virtual page to the given physical frame.
 *
 * @param pfn The physical frame to restore the data to.
 * @param vpn The virtual page number of the page to restore.
 * @param pid The process identifer of the page's owner process.
 */
void page_from_disk(pfn_t pfn, vpn_t vpn, int pid) {
   struct swap_page *curr, *prev = NULL;


   /* Search the swapfile */
   curr = swapfile;
   while (curr != NULL) {
      if (curr->pid == pid && curr->vpn == vpn) {
         break;
      } else {
         prev = curr;
         curr = curr->next;
      }
   }

   /* Provide a zeroed page when not saved on disk */
   if (curr == NULL) {
      memset(&memory[pfn * page_size], 0, page_size);
      return;
   } else {  /* Otherwise retrieve info from "disk" */
      count_diskaccesses++;
      memcpy(&memory[pfn * page_size], curr->data, page_size);

      /* Unlink the swap page */
      if (prev == NULL) {
         swapfile = curr->next;
      } else {
         prev->next = curr->next;
      }
      free(curr->data);
      free(curr);
   }
}

/*******************************************************************************
 * Free the swap file.
 */
void swapfile_free(void) {
  struct swap_page *curr, *prev;

  /* Walk the swapfile linked list and free pages */
  curr = swapfile;
  while (curr != NULL) {
    prev = curr;
    curr = curr->next;
    free(prev->data);
    free(prev);
  }
}
