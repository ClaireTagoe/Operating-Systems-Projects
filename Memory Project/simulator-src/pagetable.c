
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pagetable.h"

/*******************************************************************************
 * Current page table register
 */
pte_t   *current_pagetable; /* CPU page register */
