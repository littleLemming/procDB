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
 * @brief max length for a line of user input
 */
#define LINE_SIZE (1024)


 /**
 * @brief Name of the program
 */
static const char *progname = "procdb-client"; /* default name */

 /** 
 * @brief variable that gets set as soon as a signal gets received
 */
volatile sig_atomic_t quit = 0;

/**
 * @brief semaphore for request
 */
sem_t *request;

/**
 * @brief semaphore for response
 */
sem_t *response;

/**
 * @brief semaphore for cleanup
 */
sem_t *cleanup;

/**
 * @brief semaphore for interaction_started
 */
sem_t *interaction_started;


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

/**
 * @brief prints an information what an valid command should look like
 */
static void print_invalid_command(void);


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
    if (argc != 1) {
        bail_out(EXIT_FAILURE, "no arguments - usage: procdb-client");
    }
}

static void signal_handler(int sig) {
    quit = 1;
}

static void print_invalid_command(void) {
    printf("INVALID COMMAND: command must look like PID INFO - PID = {min, max, sum, avg, i} where i is a valid int >= 0, INFO = {cpu, mem, time, command}\ncommand can only appear with a specific pid\n");
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

    /* check if shared memory object exists */
    /* check if semaphore exists */

    /* via stdin get commands from user to send to server */
    /* as soon as client received command it gets sent to the server, proccessed there and the client reads the reply and prints it */
    char* line = malloc((size_t) LINE_SIZE);
    while(fgets(line, LINE_SIZE-1 , stdin) != NULL) {
        /* check if the command that got entered was valid */
        char *s = strtok(line," ");
        /* s should either be an int or min, max, sum, avg */
        int pid = -1;
        int pid_cmd = -1;
        if (strcmp("min", s) == 0) {
            pid_cmd = 0;
        } else if (strcmp("max", s) == 0) {
            pid_cmd = 1;
        } else if (strcmp("sum", s) == 0) {
            pid_cmd = 2;
        } else if (strcmp("avg", s) == 0) {
            pid_cmd = 3;
        }
        else {
            char *endptr = NULL;
            int i = strtol(s, &endptr, 10);
            if (endptr == s || strcmp("", endptr) != 0 || ((i == LONG_MAX || i == LONG_MIN) && errno == ERANGE)) {
                print_invalid_command();
                continue;
            }
            pid = i;
        }
        if (pid_cmd != -1 && pid == -1) {
            pid = -2;
        }
        if (pid == -1 && pid_cmd == -1) {
            print_invalid_command();
            continue;
        }
        /* s should either be cpu, mem, time or command */
        s = strtok(NULL," ");
        if (s[(strlen(s)-1)] == '\n') {
            char *pos = s+strlen(s)-1;
            *pos = '\0';
        }
        int info = -1;
        if (strcmp("cpu", s) == 0) {
            info = 0;
        } else if (strcmp("mem", s) == 0) {
            info = 1;
        } else if (strcmp("time", s) == 0) {
            info = 2;
        } else if (strcmp("command", s) == 0) {
            info = 3;
        }
        if (info == -1) {
            print_invalid_command();
            continue;
        }
        if (info == 3 && pid_cmd != -1) {
            print_invalid_command();
            continue;
        }
        s = strtok(NULL," ");
        if (s != NULL) {
            print_invalid_command();
            continue;
        }
        printf("%d\n", pid);
        printf("%d\n", pid_cmd);
        printf("%d\n", info);

        /* write the command to the server */

        /* read the response and print */
    }
    free(line);

    /* EOF file got read - shut down client */
    free_resources();
    return 0;
}