/**
 * @file procdb.h
 *
 * @author Ulrike Schaefer 1327450
 * 
 * @brief procdb is a server-client program where the server is a process-database and the client is the interface to the server for the user
 *
 * @date 19.04.2017
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <limits.h>
#include <semaphore.h>
#include <fcntl.h> 
#include <sys/mman.h>


#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif

 /**
 * @brief Length of an array
 */
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))

 /**
  * @brief Value for True
  */
#define TRUE (1)

 /**
  * @brief Value for False
  */
#define FALSE (0)

  /* 
   * @brief persmissions for semaphores & shared memory
   */
#define PERMISSION (0600)

/*
 * @brief location of the server-control semaphore for clients to connect to
 */ 
#define SEM_SERVER "/procdb_server_control_sem"

/*
 * @brief location of the server-control shared memory for clients to connect to
 */ 
#define SHM_SERVER "/procdb_server_control_shm"

struct shm_struct {
    /* at first set to -1, the client sets it to either -2 if pid_cmd should be used or to the numeric value of the proccess id */
    int pid;
    /* at first set to -1, if the client sets pid to -2 this value gets used - if set to 0 it means min, to 1 max, to 2 sum, to 3 avg */
    int pid_cmd;
    /* at first set to -1, represents what information the client wants, 0 - cpu, 1 - mem, 2 - time, 3 - command */
    int info;
    /* at first set to NULL, this is what the server returns to the client - numbers and char* get both returned as a char* as the client does not need to proccess it */
    char *value;
    /* after the client read the information the server sent he writes 1 to ack, the server sets it back to 0 after having cleaned up */
    int ack; 
};