/* Simple example of using execvp with helpful function to read a line from
 * stdin and parse it into an array of individual tokens
 * Author: Sherri Goings
 * Last Modified: 1/18/2014
 */
// Huiji Ying
// Claire Tagoe

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
char** readLineOfWords();
int execute(int i, char** words);
int childPro(int i, char** words);

int main() {

  while (1) {

    printf("enter a shell command (e.g. ls): ");
    fflush(stdout);

    char** words = readLineOfWords();

    // prints the tokens in the array separated by spaces
    int i = 0;
    printf("\nyou entered: ");
    while ((words[i] != NULL)) {

      printf("%s ", words[i++]);
      fflush(stdout);
    }
    printf("\n\n");
    fflush(stdout);

    // execute command in words[0] with arguments in array words
    // by convention first argument is command itself, last argument must be
    // NULL
    int pid = fork();
    // printf("%d", pid);
    // fflush( stdout );

    if (pid == 0) {
      // Child Process
      childPro(i, words);
    } else {
      // printf("%s\n", words[i - 1]);
      // fflush(stdout);
      // Parent Process
      if (strchr(words[i - 1], '&')) {
        // printf("%s\n", "not waiting");
        // fflush(stdout);
      } else {
        // printf("%s\n", "waiting");
        // fflush(stdout);
        waitpid(pid, NULL, 0);
        // printf("%s\n", "done waiting");
        // fflush(stdout);
      }
    }

    // execvp(words[0], words);

    // execvp replaces current process so should never get here!
    // printf("Will this get printed? No.\n");
  }

  return 0;
}

int childPro(int i, char** words) {
  // Child Process
  bool p_present = false;

  for (int j = 0; j < i; j++) {
    // Check if need to pipe
    if (strchr(words[j], '|')) {
      char** sub_instruction1 = (char**)malloc(100 * sizeof(char*));
      p_present = true;

      for (int idx = 0; idx < j; idx++) {
        sub_instruction1[idx] = (char*)malloc(10 * sizeof(char*));
        strncpy(sub_instruction1[idx], words[idx], 40);
        // printf("%d\n", idx);
        // printf("%s\n", sub_instruction[idx]);
        // fflush(stdout);
      }

      // execute(j, sub_instruction1);

      char** sub_instruction2 = (char**)malloc(100 * sizeof(char*));
      int t = 0;
      int s = j + 1;

      while (words[s] != NULL) {
        sub_instruction2[t] = (char*)malloc(10 * sizeof(char*));
        strncpy(sub_instruction2[t], words[s], 40);
        // printf("%d\n", t);
        // printf("%s\n", sub_instruction2[t]);
        // fflush(stdout);
        t++;
        s++;
      }
      // //execute(j, sub_instruction1);
      //           execute(t, sub_instruction2);
      int pipefd[2];
      pipe(pipefd);
      int childpid = fork();
      if (childpid == 0) {
        close(pipefd[0]);
        p_present = true;
        // printf("%s\n", "in parent");
        // fflush(stdout);
        dup2(pipefd[1], 1);
        execute(j, sub_instruction1);
        close(pipefd[1]);
      } else {
        close(pipefd[1]);
        dup2(pipefd[0], 0);
        // read(pipefd[0], stdin, strlen(pipefd[0]));
        // printf("%s\n", "in child");
        // fflush(stdout);

        childPro(t, sub_instruction2);

        close(pipefd[0]);
      }

      //
    }
  }

  if (!p_present) {
    execute(i, words);
  }
  return 1;
}

int execute(int i, char** words) {
  for (int j = 0; j < i; j++) {
    // printf("%s", words[j]);
    // fflush(stdout);
  }
  if (strchr(words[i - 1], '&')) {
    words[i - 1] = NULL;
  }
  if (i > 2) {
    if (strchr(words[i - 2], '>')) {
      // 			  printf("%s\n", "We are in > if");
      // 			  fflush(stdout);
      int file = open(words[i - 1], O_CREAT | O_WRONLY, 0644);
      // 			  printf("%d\n", file);
      dup2(file, 1);
      words[i - 2] = NULL;
      i = i - 2;
    }
  }
  if (i > 2) {
    if (strchr(words[i - 2], '<')) {
      // 			  printf("%s\n", "We are in < if");
      // 			  fflush(stdout);
      int file = open(words[i - 1], O_CREAT | O_RDONLY, 0644);
      // 			  printf("%d\n", file);
      dup2(file, 0);
      words[i - 2] = NULL;
    }
  }
  // printf("%s\n", words[0]);
  execvp(words[0], words);
  exit(0);
  return 1;
}

/*
 * reads a single line from terminal and parses it into an array of
 * tokens/words
 * by
 * splitting the line on spaces.  Adds NULL as final token
 */
char** readLineOfWords() {

  // A line may be at most 100 characters long, which means longest word is
  // 100
  // chars,
  // and max possible tokens is 51 as must be space between each
  size_t MAX_WORD_LENGTH = 100;
  size_t MAX_NUM_WORDS = 51;

  // allocate memory for array of array of characters (list of words)
  char** words = (char**)malloc(MAX_NUM_WORDS * sizeof(char*));
  int i;
  for (i = 0; i < MAX_NUM_WORDS; i++) {
    words[i] = (char*)malloc(MAX_WORD_LENGTH);
  }

  // read actual line of input from terminal
  int bytes_read;
  char* buf;
  buf = (char*)malloc(MAX_WORD_LENGTH + 1);
  bytes_read = getline(&buf, &MAX_WORD_LENGTH, stdin);

  // take each word from line and add it to next spot in list of words
  i = 0;
  char* word = (char*)malloc(MAX_WORD_LENGTH);
  word = strtok(buf, " \n");
  while (word != NULL && i < MAX_NUM_WORDS) {
    strcpy(words[i++], word);
    word = strtok(NULL, " \n");
  }

  // check if we quit because of going over allowed word limit
  if (i == MAX_NUM_WORDS) {
    printf("WARNING: line contains more than %d words!\n", (int)MAX_NUM_WORDS);
  } else
    words[i] = NULL;

  // return the list of words
  return words;
}
