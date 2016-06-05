/* Very simple example of using syscalls 
 * pipe(), read(), and write()
 * Author: Sherri Goings
 * Last modified: 1/18/2014
 */
 
#include <stdlib.h>
#include <stdio.h>


int main() {
  int desp[2];                 // array of 2 file descriptors
  const int MAX_LENGTH = 128;  // max length of line
  
  // after call to pipe desp[0] will be a file that reads from
  // the pipe, desp[1] will be a file that writes to the pipe
  pipe(desp);

  char* s = "send me through the pipe!";

  // write string s to file desp[1] up to MAX_LENGTH characters
  write(desp[1], s, MAX_LENGTH);

  char line[MAX_LENGTH];

  // read up to MAX_LENGTH characters from file desp[0] and store
  // in string line
  read(desp[0], line, MAX_LENGTH);

  printf("read from pipe: \"%s\" \n", line);

  // close both files
  close(desp[0]);
  close(desp[1]);
  
  return 0;
}
