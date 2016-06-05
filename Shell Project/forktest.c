/*
    forktest.c
    
    Started by Jeff Ondich on 3/26/96
    Last modified 1/18/2014 by Sherri Goings
    
    This program gives a simple example of fork, and how
    a process can create a child process.
*/

#include    <stdio.h>
#include    <unistd.h>

const long limit = 1000000000;
const long frequency = 1000000;

int main()
{
    long        i;

    // fork splits process into 2 identical processes that both continue
    // running from point where fork() returns. Only difference is return
    // value - 0 to the child process, pid of child to the parent process  
    int pid = fork();

    // if 0 is returned, execute code for child process
    if( pid == 0 )
    {
        for( i=0; i < limit; i++ )
        {
            if( i % frequency == 0 )
            {
                 printf( "I'm the child - %ld\n", i );
                 fflush( stdout );
            }
        }
    }
    // otherwise execute code for parent process
    else
    {
        for( i=0; i < limit; i++ )
        {
            if( i % frequency == 0 )
            {
              printf( "I'm the parent of child with pid=%d - %ld\n", pid, i );
                fflush( stdout );
            }
        }
    }


    return 0;
}
