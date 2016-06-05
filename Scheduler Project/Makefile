# Makefile
# CS 2200 PRJ4

src=student.c os-sim.c process.c
obj=student.o os-sim.o process.o
inc=student.h os-sim.h process.h
misc=Makefile
target=os-sim
cflags=-g -O0
lflags=-lpthread

all: $(target)

$(target) : $(obj) $(misc)
	gcc $(cflags) $(lflags) -o $(target) $(obj)

%.o : %.c $(misc) $(inc)
	gcc $(cflags) -c -o $@ $<

clean:
	rm -f $(obj) $(target)
