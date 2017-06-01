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
 * @brief max length for a line in input-file
 */
#define LINE_SIZE (1024)


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
 * @brief struct that represents a entry in the input file
 */
struct process {
    int pid;
    int p_cpu;
    int p_mem;
    int p_time;
    char *p_command;
};
typedef struct process process;

/**
 * @brief list of processes initially read in from the input-list
 */
struct process *processes = NULL;

/**
 * @brief length of list of processes initially read in from the input-list
 */
int length_porccesses = 0;

/**
 * @brief count of proccesses in proccess list
 */
int count_porccesses = 0;


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
    if (processes != NULL) {
        free(processes);
    }
}

static void parse_args(int argc, char **argv) {
    if(argc > 0) {
        progname = argv[0];
    }
    if (argc != 2) {
        bail_out(EXIT_FAILURE, "needs input-file - usage: procdb-server input-file");
    }
    /* open input-file and read line by line - save content */
    FILE *input_file;
    input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        bail_out(EXIT_FAILURE, "could not open file - enter valid file - usage: procdb-server input-file");
    }
    char *line = malloc((size_t) LINE_SIZE);
    while (fgets(line, (size_t) LINE_SIZE, input_file) != NULL) {
        struct process p;

        char *s = strtok(line,",");
        int cnt = 0; // 0 - pid, 1 - cpu, 2 - mem, 3 - time, 4 - command
        int i;
        char *endptr = NULL;
        while (s != NULL) {
            if (cnt > 4) {
                bail_out(EXIT_FAILURE, "too many arguments in input-file");
            }
            if (cnt != 4) {
                endptr = NULL;
                i = strtol(s, &endptr, 10);
                if (endptr == s || ((i == LONG_MAX || i == LONG_MIN) && errno == ERANGE)) {
                    bail_out(EXIT_FAILURE, "invalid int in input-file");
                }
            } 
            switch (cnt) {
            case 0:
                p.pid = i;
                break;
            case 1:
                p.p_cpu = i;
                break;
            case 2:
                p.p_mem = i;
                break;
            case 3:
                p.p_time = i;
                break;
            case 4:
                p.p_command = strdup(s);
                break;
            default:
                bail_out(EXIT_FAILURE, "too many arguments in one line in input-file");
            }
            s = strtok(NULL,",");
            ++ cnt;
        }

        if (count_porccesses+1 >= length_porccesses) {
            length_porccesses += 5;
            processes = (struct process*) realloc(processes, length_porccesses*sizeof(struct process));
        }
        processes[count_porccesses] = p;
        count_porccesses ++;
    } if (feof(input_file) == 0) {
        bail_out(EXIT_FAILURE, "could not properly read input-file");
    }
    if (fclose(input_file) != 0) {
        bail_out(EXIT_FAILURE, "could not properly close input-file after read");
    }
    for (int i = 0; i < count_porccesses; ++i) {
        printf("proccess - pid: %d, cpu: %d, mem: %d, time: %d, command: %s\n", processes[i].pid, processes[i].p_cpu, processes[i].p_mem, processes[i].p_time, processes[i].p_command);
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

    /* reserve list of processes to save stuff from input-file in */
    processes = malloc(sizeof(struct process)*5);
    length_porccesses = 5;

    /* parse arguments */
    parse_args(argc, argv);


    /*for (int i = 0; i < count_porccesses; ++i) {
        printf("proccess - pid: %d, cpu: %d, mem: %d, time: %d, command: %s\n", processes[i].pid, processes[i].p_cpu, processes[i].p_mem, processes[i].p_time, processes[i].p_command);
    }*/

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