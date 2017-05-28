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
#define TRUE 1

 /**
  * @brief Value for False
  */
#define FALSE 0