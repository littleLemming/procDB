/**
 * @file client.c
 *
 * @author Ulrike Schaefer 1327450
 * 
 * @brief the client is a part of procdb - it is the interface for the user to access the process-database
 *
 * @details the client communicate with the server via shared memory. cpu, mem, time or command can be asked of the server for every process.
 *
 * @date 21.05.2017
 * 
 */

#include "procdb.h"


 /**
 * @brief Name of the program
 */
static const char *progname = "procdb-client"; /* default name */


 /**
 * @brief terminate program on program error
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * @brief free allocated resources
 */
static void free_resources(void);

/**
 * @brief Parse command line options
 * @param argc The argument counter
 * @param argv The argument vector
 * @param options Struct where parsed arguments are stored
 */
static void parse_args(int argc, char **argv);

/**
 * @brief Signal handler for SIGINT & SIGTERM which should shut down the client
 * @param sig Signal number catched
 */
static void signal_handler(int sig);


static void bail_out(int exitcode, const char *fmt, ...) {
    va_list ap;

    (void) fprintf(stderr, "%s: ", progname);
    if (fmt != NULL) {
        va_start(ap, fmt);
        (void) vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    if (errno != 0) {
        (void) fprintf(stderr, ": %s", strerror(errno));
    }
    (void) fprintf(stderr, "\n");

    free_resources();
    exit(exitcode);
}

static void free_resources(void) {
}

static void parse_args(int argc, char **argv) {
    if(argc > 0) {
        progname = argv[0];
    }
    if (argc != 1) {
        bail_out(EXIT_FAILURE, "no arguments - usage: procdb-client");
    }
}

static void signal_handler(int sig) {
    quit = 1;
}

/**
 * main
 * @brief starting point of program
 * @param argc number of program arguments
 * @param argv program arguments
 */
int main(int argc, char *argv[]) {

    /* setup signal handlers */
    const int signals[] = {SIGINT, SIGTERM};
    struct sigaction s;

    s.sa_handler = signal_handler;
    s.sa_flags   = 0;
    if(sigfillset(&s.sa_mask) < 0) {
        bail_out(EXIT_FAILURE, "sigfillset");
    }
    for(int i = 0; i < COUNT_OF(signals); i++) {
        if (sigaction(signals[i], &s, NULL) < 0) {
            bail_out(EXIT_FAILURE, "sigaction");
        }
    }

    /* parse arguments */
    parse_args(argc, argv);

    /* connect to server */
    /* check if semaphore exists */

    /* check if shared memory object exists */


    
    return 0;
}