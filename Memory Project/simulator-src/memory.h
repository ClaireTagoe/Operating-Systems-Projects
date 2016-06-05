#ifndef _MEMORY_H_
#define _MEMORY_H_

/*******************************************************************************
 * Loads the data given by the virtual address, using the virutal -> physical
 * translation system of the CPU.
 *
 * @param va The virtual address to read.
 * @return The data at the given address.
 */
word_t mem_load(vaddr_t va);

/*******************************************************************************
 * Stores the data given by the virtual address, using the virutal -> physical
 * translation system of the CPU.
 *
 * @param va The virtual address to write.
 * @param data The data to write to the address.
 */
void mem_store(vaddr_t va, word_t data);

/*******************************************************************************
 * Initializes main memory (RAM)
 */
void mem_init(void);

/*******************************************************************************
 * Frees all the memory used to represent physical memory
 */
void mem_free(void);

#endif/*_MEMORY_H_*/
