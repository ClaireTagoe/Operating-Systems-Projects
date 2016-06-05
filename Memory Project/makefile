# Makefile for CS2200 - Project 3: Virtual Memory
# Author: Sean Caulfield <sean.caulfield@cc.gatech.edu>
# RCS: Makefile,v 1.17 2004/04/20 03:52:23 caulfiel Exp

# NOTE: You should NOT have to modify this file. Break at your own risk!

# these are the two directories where source code can be located
STUDENT   = student-src
SIMULATOR = simulator-src

CC      = gcc
OPTIONS = -O2 -I$(STUDENT) -I$(SIMULATOR) # Production options
#OPTIONS = -O0 -g -I$(STUDENT) -I$(SIMULATOR) # Debugging Options
CFLAGS  = $(OPTIONS) -Wall -ansi -pedantic -pipe
SUBMIT  = $(STUDENT) $(SIMULATOR) Makefile references

STUDENT_OBJS = page-fault.o \
               page-replacement.o \
               page-lookup.o \
               emat.o \
               tlb-lookup.o

SIMULATOR_OBJS = global.o \
                 memory.o \
                 pagetable.o \
                 process.o \
                 swapfile.o \
                 statistics.o \
                 sim.o \
                 tlb.o

ALL_OBJS = $(STUDENT_OBJS:%.o=$(STUDENT)/%.o) \
           $(SIMULATOR_OBJS:%.o=$(SIMULATOR)/%.o)

ALL = vm-sim
all: $(ALL)

vm-sim: $(ALL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Use this target to collect all the files to submit to webwork
submit: clean
	tar cvfz p3-submit.tar.gz $(SUBMIT)

.PHONY: clean
clean:
	rm -rf $(ALL) $(STUDENT)/*.o $(SIMULATOR)/*.o core*

# everything below this line describes the dependencies that exist.
$(SIMULATOR)/global.o: $(SIMULATOR)/global.h $(SIMULATOR)/global.c
$(SIMULATOR)/memory.o: $(SIMULATOR)/memory.h $(SIMULATOR)/memory.c \
                       $(SIMULATOR)/tlb.h    $(SIMULATOR)/statistics.h \
                       $(SIMULATOR)/useful.h $(SIMULATOR)/types.h
$(SIMULATOR)/pagetable.o: $(SIMULATOR)/pagetable.c $(SIMULATOR)/pagetable.h \
                          $(SIMULATOR)/types.h
$(SIMULATOR)/swapfile.o:    $(SIMULATOR)/swapfile.c    $(SIMULATOR)/swapfile.h \
                          $(SIMULATOR)/types.h         $(SIMULATOR)/global.h \
                          $(SIMULATOR)/useful.h        $(SIMULATOR)/statistics.h
$(SIMULATOR)/process.o:   $(SIMULATOR)/process.c   $(SIMULATOR)/process.h \
                          $(SIMULATOR)/global.h    $(SIMULATOR)/useful.h \
                          $(SIMULATOR)/tlb.h       $(SIMULATOR)/pagetable.h \
                          $(SIMULATOR)/types.h
$(SIMULATOR)/sim.o:       $(SIMULATOR)/sim.c        $(SIMULATOR)/global.h \
                          $(SIMULATOR)/process.h    $(SIMULATOR)/memory.h \
                          $(SIMULATOR)/statistics.h $(SIMULATOR)/swapfile.h \
                          $(SIMULATOR)/tlb.h        $(SIMULATOR)/useful.h \
                          $(SIMULATOR)/pagetable.h  $(SIMULATOR)/types.h
$(SIMULATOR)/statistics.o: $(SIMULATOR)/statistics.h $(SIMULATOR)/statistics.c
$(SIMULATOR)/tlb.o: $(SIMULATOR)/tlb.c       $(SIMULATOR)/tlb.h \
                    $(SIMULATOR)/global.h    $(SIMULATOR)/useful.h \
                    $(SIMULATOR)/types.h
$(STUDENT)/emat.o:  $(SIMULATOR)/statistics.h $(STUDENT)/emat.c
$(STUDENT)/page-fault.o: $(SIMULATOR)/types.h    $(SIMULATOR)/process.h \
                         $(SIMULATOR)/global.h   $(SIMULATOR)/swapfile.h \
                         $(STUDENT)/page-fault.c $(SIMULATOR)/pagetable.h
$(STUDENT)/page-lookup.o: $(SIMULATOR)/pagetable.h $(STUDENT)/page-lookup.c \
                          $(SIMULATOR)/swapfile.h    $(SIMULATOR)/statistics.h \
                          $(SIMULATOR)/types.h
$(STUDENT)/page-replacement.o: $(SIMULATOR)/types.h  $(SIMULATOR)/pagetable.h \
                               $(SIMULATOR)/global.h $(SIMULATOR)/process.h \
                               $(STUDENT)/page-replacement.c
$(STUDENT)/tlb-lookup.o: $(STUDENT)/tlb-lookup.c  $(SIMULATOR)/tlb.h \
                         $(SIMULATOR)/pagetable.h $(SIMULATOR)/types.h

