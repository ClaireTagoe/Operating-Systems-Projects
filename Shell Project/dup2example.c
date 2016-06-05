/* Very simple example of dup2 system call
 * Author: Sherri Goings
 * Last Modified: 1/17/2014
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

int main() {
  int pid, status;

  // new file descriptor, O_CREAT means will create a new file
  // if given file does not exist, O_WRONLY means file is open only
  // for writing to
  int newfd = open("test.out", O_CREAT|O_WRONLY, 0644);

  printf("This goes to the standard output\n");
  fflush( stdout );

  // standard output is file descriptor 1, so this call to
  // dup2 will close the standard output and make newfd file
  // descriptor 1 in the file table instead, causing all printf
  // statements to write to newfd
  dup2(newfd, 1);

  printf("This goes to file descriptor 1, which is now the new file");
  
  return 0;
}
