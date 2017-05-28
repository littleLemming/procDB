/**
 * @file server.c
 *
 * @author Ulrike Schaefer 1327450
 * 
 * @brief the client is a part of procdb - it is the interface for the user to access the process-database
 *
 * @details the server communicate with the clients via exactly one shared memory object. cpu, mem, time or command can be asked of the server for every process. the processes get identified by their PID
 *
 * @date 21.05.2017
 * 
 */

#include "procdb.h"


 /**
 * @brief Name of the program
 */
static const char *progname = "procdb-server"; /* default name */

 /** 
 * @brief variable that gets set as soon as a SIGINT or SIGTERM signal gets received
 */
volatile sig_atomic_t quit = 0;

 /** 
 * @brief variable that gets set as soon as a SIGUSR1 signal gets received
 */
volatile sig_atomic_t print_db = 0;


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
 * @brief Signal handler for SIGINT & SIGTERM which should shut down the server
 * @param sig Signal number catched
 */
static void signal_quit_handler(int sig);

/**
 * @brief Signal handler for SIGUSR1 which should print all data in the database
 * @param sig Signal number catched
 */
static void signal_print_db_handler(int sig);


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
    printf("freeing resources\n");
}

static void parse_args(int argc, char **argv) {
    if(argc > 0) {
        progname = argv[0];
    }
    if (argc != 2) {
        bail_out(EXIT_FAILURE, "needs input-file - usage: procdb-server input-file");
    }
}

static void signal_quit_handler(int sig) {
    quit = 1;
}

static void signal_print_db_handler(int sig) {
    print_db = 1;
}


/**
 * main
 * @brief starting point of program
 * @param argc number of program arguments
 * @param argv program arguments
 */
int main(int argc, char *argv[]) {

    /* setup signal handlers */
    const int quit_signals[] = {SIGINT, SIGTERM};
    struct sigaction s_q;

    const int printdb_signals[] = {SIGUSR1};
    struct sigaction s_p;

    s_q.sa_handler = signal_quit_handler;
    s_q.sa_flags   = 0;
    if(sigfillset(&s_q.sa_mask) < 0) {
        bail_out(EXIT_FAILURE, "sigfillset - quit");
    }
    for(int i = 0; i < COUNT_OF(quit_signals); i++) {
        if (sigaction(quit_signals[i], &s_q, NULL) < 0) {
            bail_out(EXIT_FAILURE, "sigaction - quit");
        }
    }

    s_p.sa_handler = signal_print_db_handler;
    s_p.sa_flags   = 0;
    if(sigfillset(&s_p.sa_mask) < 0) {
        bail_out(EXIT_FAILURE, "sigfillset - print");
    }
    for(int i = 0; i < COUNT_OF(printdb_signals); i++) {
        if (sigaction(printdb_signals[i], &s_p, NULL) < 0) {
            bail_out(EXIT_FAILURE, "sigaction - print");
        }
    }

    /* parse arguments */
    parse_args(argc, argv);

    /* setup shared memory & semaphores */

    /* wait for requests of clients, and write back answers */
    while (TRUE) {
        if (quit == 1) {
            printf("caught signal - shutting down\n");
            break;
        }
        if (print_db == 1) {
            printf("printing database:\n");
            print_db = 0;
        }
    }

    free_resources();
    return 0;
}