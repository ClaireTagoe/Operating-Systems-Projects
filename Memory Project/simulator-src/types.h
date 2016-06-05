#ifndef _TYPES_H_
#define _TYPES_H_

#include <netinet/in.h> /* For uintXX_t types */

/*******************************************************************************
 * Virtual addresses are 16 bits.
 */
typedef uint16_t vaddr_t;

/*******************************************************************************
 * Physical addresses are 16 bits.
 */
typedef uint16_t paddr_t;

/*******************************************************************************
 * Virtual page numbers can be up to 16 bits. For pedantic reasons.
 */
typedef uint16_t vpn_t;

/*******************************************************************************
 * Physical frame numbers can be up to 16 bits. For pedantic reasons.
 */
typedef uint16_t pfn_t;

/*******************************************************************************
 * This machine is byte addressed, so an unsigned char will suffice.
 */
typedef unsigned char word_t;

#endif
