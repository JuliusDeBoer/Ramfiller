#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Terrible hack to have both windows and unix threads

#ifdef WIN32 // Windows
#include <windows.h>
#define THREAD_ID HANDLE
#define SLEEP(t) Sleep(t)

#else // Unix
#include <pthread.h>
#include <unistd.h>
#define THREAD_ID pthread_t
#define SLEEP(t) usleep(t)
#endif

#ifndef PROJ_VERSION
#define PROJ_VERSION "UNKNOWN"
#endif

typedef struct Block {
  uint64_t *data;
  uint64_t index;
  uint64_t ceil;

  struct Block *next;
} Block;

typedef struct Thread {
  THREAD_ID id;
  bool stop;
  int delay;

  Block *block;
} Thread;

/// Prints a usage message
///
/// Usage:
/// \code
/// 	printUsage(argv[0])
/// \endcode
///
/// \param programName The name of the program.
void printUsage(char *programName) {
  printf("RamFiller %s\n"
         "Does what it says on the tin. It fills up ram\n\n"
         "USAGE:\n"
         "\t%s [GB OF RAM TO FILL]\n\n"
         "ARGUMENTS:\n"
         "\t-h --help\n"
         "\t    Print this help message\n\n"
         "\t-b --block-size\n"
         "\t    Set the size of a block in bytes\n\n"
         "\t-u --dont-update\n"
         "\t    Dont update memory\n\n"
         "\t-d --delay\n"
         "\t    Set delay between updates in ms\n\n"
         "CREDITS:\n"
         "\t    Julius de Boer\n",
         PROJ_VERSION, programName);
}

/// Keep refreshing the memory block
void *keepAlive(void *arg) {
  Thread *thread = arg;

  while (!thread->stop) {
    Block *block = thread->block;

    while (block) {
      int num = rand();
      for (block->index = 0; block->index <= block->ceil; block->index += 1) {
        block->data[block->index] = num;
        SLEEP(10);
        if (thread->stop) {
          return thread;
        }
      }
      block = block->next;
    }
  }
  return thread;
}

void startThread(Thread *thread) {
  thread->stop = false;
#ifdef WIN32 // Windows
  thread->id = CreateThread(NULL, 0, keepAlive, thread, 0, NULL);
#else // Unix
  pthread_create(&thread->id, NULL, keepAlive, thread);
#endif
  return;
}

void destroyThread(Thread *thread) {
  thread->stop = true;
  printf("Waiting for thread to stop...");
  fflush(stdout);
#ifdef WIN32 // Windows
  WaitForSingleObject(thread->id, 10000);
#else // Unix
  pthread_join(thread->id, NULL);
#endif
  printf("Done!\n");
  return;
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    printUsage(argv[0]);
    return EXIT_FAILURE;
  }

  long sizeInGib = -1;
  uint64_t blockSize = 1073741824;
  bool update = true;
  Thread thread;
  thread.delay = 10;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      printUsage(argv[0]);
      exit(EXIT_SUCCESS);
    } else if (strcmp(argv[i], "-b") == 0 ||
               strcmp(argv[i], "--block-size") == 0) {
      i++;
      if (i < argc) {
        printf("%s needs a second argument!\n", argv[i - 1]);
        exit(EXIT_SUCCESS);
      }
      blockSize = strtol(argv[i], NULL, 0);
      if (blockSize <= 0) {
        printf("Block size has to be a possitive integers\n");
        exit(EXIT_FAILURE);
      }
    } else if (strcmp(argv[i], "-u") == 0 ||
               strcmp(argv[i], "--dont-update") == 0) {
      update = false;
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--delay") == 0) {
      i++;
      if (i >= argc) {
        printf("%s needs a second argument!\n", argv[i - 1]);
        exit(EXIT_FAILURE);
      }
      thread.delay = strtol(argv[i], NULL, 0);
    } else {
      sizeInGib = strtol(argv[i], NULL, 0);
      if (sizeInGib <= 0) {
        printf("Size has to be a possitive integers\n");
        exit(EXIT_FAILURE);
      }
    }
  }

  if (sizeInGib <= 0) {
    printf("Please enter a size!\n");
    exit(EXIT_FAILURE);
  }

  printf("Allocating %luGB of memory...", sizeInGib);
  fflush(stdout);

  thread.block = malloc(sizeof(Block));
  Block *block = thread.block;

  block->ceil = blockSize / sizeof(uint64_t);
  block->data = malloc(blockSize);

  for (int i = 1; i < sizeInGib; i++) {
    block->next = malloc(sizeof(Block));
    block = block->next;

    block->ceil = blockSize / sizeof(uint64_t);
    block->data = malloc(blockSize);

    if (block->data == NULL) {
      char msg[64];
      snprintf(msg, 64, "\nError while allocating memory for block %d\n", i);
      perror(msg);
      exit(EXIT_FAILURE);
    }
  }

  block->next = NULL;

  printf("Done!\n");
  printf("Filling up blocks...");
  fflush(stdout);

  block = thread.block;

  const uint64_t num = rand();
  while (block) {
    for (block->index = 0; block->index <= block->ceil; block->index += 1) {
      block->data[block->index] = num;
    }
    block = block->next;
  }

  printf("Done!\n");

  if (update) {
    startThread(&thread);
  }

  printf("Press enter to free memory...");
  getchar();

  if (update) {
    destroyThread(&thread);
  }

  printf("Deallocating memory...");
  fflush(stdout);

  block = thread.block;
  Block *next;

  while (block != NULL) {
    next = block->next;
    free(block);
    block = next;
  }

  return EXIT_SUCCESS;
}
