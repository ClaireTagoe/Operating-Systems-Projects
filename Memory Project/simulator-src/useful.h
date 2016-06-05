#include <string.h>
#include <errno.h>

#ifndef PRJ3_USEFUL_H
#define PRJ3_USEFUL_H

/* 
 * Prints an error message to stderr, including the file and line where it was
 * generated.
 */
#define ERROR(s) fprintf(stderr, "%s@%d: %s\n", __FILE__, __LINE__, s)

/*
 * As ERROR() above, but also includes the numeric and string value of errno.
 */
#define PERROR(s) fprintf(stderr, "%s@%d: %s: %s(%d)\n", __FILE__, __LINE__, s,\
    strerror(errno), errno)

#endif
