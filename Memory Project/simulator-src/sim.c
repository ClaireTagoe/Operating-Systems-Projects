/**
 * CS2200 Project 3: Virtual Memory
 *
 * sim.c  The actual state machine for the VM simulator.
 *
 *        NOTE: You should NOT have to modify this file!
 *
 * Author: Sean Caulfield <sean.caulfield@cc.gatech.edu>
 * RCS: $Id: sim.c,v 1.14 2005/03/11 21:05:10 sean Exp $
 *
 */

#define _GNU_SOURCE /* To get getopt(3) working properly */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h> /* For uint8_t */

#include "global.h"
#include "process.h"
#include "memory.h"
#include "statistics.h"
#include "swapfile.h"
#include "tlb.h"
#include "useful.h"

/******************************************************************************
 *                                M A C R O S                                 *
 ******************************************************************************/

/**
 * Description of command-line flags for the simulator.
 */
#define HELP_STR \
"[-p <page_size>] [-m <memory_size>] [-t <TLB_size>] [-j <num_jobs>] [-h] <file>\n" \
"Summary of commands (default values in parenthesis):\n"\
"-p <page_size>   Set size of VM pages in powers of two (4)\n"\
"-m <memory_size> Set size of physical memory in powers of two (8)\n"\
"-t <TLB_size>    Set number of entries in the TLB (4)\n"\
"-j <num_jobs>    Set maximum number of jobs (8)\n"\
"-h               Display this message.\n"

/**
 * Prints the command-line flags description to stderr.
 */
#define USAGE() fprintf(stderr, "Usage: %s %s", progname, HELP_STR)

/**
 * String of possible whitespace delimiters for our references file.
 */
#define WHITESPACE " \t\n\r"

/******************************************************************************
 *                       M O D U L E   V A R I A B L E S                      *
 ******************************************************************************/

/**
 * Name of our program; copied from argv[0].
 */
static char *progname;

/**
 * Name of the references file specified on the command line.
 */
static char *filename;

/******************************************************************************
 *                      P R I V A T E   F U N C T I O N S                     *
 ******************************************************************************/

/**
 * Parses command-line options and sets global variables as appropriate.
 *
 * @param argc Number of arguments from the command line.
 * @param argv Array of strings containing the command line arguments.
 */
void sim_setoptions(int argc, char **argv) {
  int c;

  progname = argv[0];
  while ((c = getopt(argc, argv, "p:m:t:j:h")) != -1) {
    switch (c) {
    case 'p':
      if (optarg == NULL || atoi(optarg) > 16 || atoi(optarg) < 1) {
        ERROR("invalid page size -- must be between 1 and 16!");
        exit(EXIT_FAILURE);
      }
      page_size = 1<<atoi(optarg);
      break;
    case 'm':
      if (optarg == NULL || atoi(optarg) > 20 || atoi(optarg) < 1) {
        ERROR("invalid memory size -- must be between 1 and 20!");
        exit(EXIT_FAILURE);
      }
      mem_size = 1<<atoi(optarg);
      break;
    case 't':
      if (optarg == NULL || atoi(optarg) > 255 || atoi(optarg) < 0) {
        ERROR("invalid TLB size -- must be between 0 and 65536!");
        exit(EXIT_FAILURE);
      }
      tlb_size = atoi(optarg);
      break;
    case 'j':
      if (optarg == NULL || atoi(optarg) > 32 || atoi(optarg) < 0) {
        ERROR("invalid max number of jobs -- must be between 0 and 32!");
        exit(EXIT_FAILURE);
      }
      max_jobs = atoi(optarg);
      break;
    case 'h':
    default:
      USAGE();
      exit(EXIT_FAILURE);
    }
  }

  if (optind >= argc) {
    ERROR("no references filename specified");
    USAGE();
    exit(EXIT_FAILURE);
  } else {
    filename = argv[optind];
  }

  if (page_size > mem_size) {
    ERROR("page size cannot be larger than physical memory!");
    exit(EXIT_FAILURE);
  }

  printf("page_size = %u bytes\n", page_size);
  printf("mem_size = %u bytes\n", mem_size);
  printf("tlb_size = %u entries\n", tlb_size);
  printf("max_jobs = %u jobs\n", max_jobs);
  printf("filename = %s\n", filename);
}

/**
 * Opens the references file, reads in each line, and dispatches the commands
 * specified as appropriate to the CPU and OS modules.
 */
void sim_readdata(void) {
  FILE *fp;
  char buff[512];

  if ((fp = fopen(filename, "r")) == NULL) {
    PERROR(filename);
    exit(EXIT_FAILURE);
  }

  /* For each line in the file... */
  while (fgets(buff, sizeof(buff), fp) != NULL) {
    char *cmd, *arg1, *arg2, *arg3;
    int pid;
    vaddr_t addr;
    word_t val, val2;

    /* Parse command and possible arguments */
    cmd  = strtok(buff, WHITESPACE);
    arg1 = strtok(NULL, WHITESPACE);
    arg2 = strtok(NULL, WHITESPACE);
    arg3 = strtok(NULL, WHITESPACE);

    switch (cmd[0]) {

    /* Create a new process */
    case '@':
      pid = atoi(arg1);
      if (pid < max_jobs) {
        printf("Forking new process %s (pid %d)...\n", arg2, pid);
        proc_fork(pid, arg2);
      } else {
        printf("Too many jobs, not forking new process!\n");
      }
      break;

    /* Load from a memory location */
    case 'l':
      pid = atoi(arg1);
      addr = atoi(arg2);
      val = atoi(arg3) % 256;
      if (pid >= max_jobs) {
        break;
      } else if (current == NULL || current->pid != pid) {
        printf("Switching to %s (pid %d)...\n", proc_getname(pid), pid);
        proc_switch(pid);
      }
      val2 = mem_load(addr);
      if (val2 != val) {
        printf(" ERROR! loaded value did not equal expected!\n");
        /* XXX: Dump a mem image here and quit */
      } else {
        printf("...OK\n");
      }
      break;

    /* Store to a memory location */
    case 's':
      pid = atoi(arg1);
      addr = atoi(arg2);
      val = atoi(arg3) % 256;
      if (pid >= max_jobs) {
        break;
      } else if (current == NULL || current->pid != pid) {
        printf("Switching to %s (pid %d)...\n", proc_getname(pid), pid);
        proc_switch(pid);
      }
      mem_store(addr, val);
      printf("...OK\n");

    /* Ignore other commands (for comments and the like */
    default:
      break;
    }
  }

  fclose(fp);
}

/**
 * Initializes the simulator by invoking the CPU and OS module initialization
 * routines.
 */
void sim_init(void) {
   mem_init();
   proc_init();
   tlb_init();
}

/**
 * Frees simulator resources by invoking the CPU and OS module release routines.
 */
void sim_free(void) {
   mem_free();
   swapfile_free();
   proc_free();
   tlb_free();
}

/**
 * Main body of the program.
 *
 * @param argc Number of arguments from the command line.
 * @param argv Array of strings containing the command line arguments.
 * @return The exit status of the program (EXIT_SUCCESS or EXIT_FAILURE).
 */
int main(int argc, char **argv) {
  sim_setoptions(argc, argv);
  sim_init();
  sim_readdata();
  display_statistics();
  sim_free();
  return EXIT_SUCCESS;
}
