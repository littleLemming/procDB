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
 * @brief semaphore for client
 */
sem_t *client;

/**
 * @brief semaphore for server
 */
sem_t *server;

/**
 * @brief semaphore for interaction_started
 */
sem_t *interaction_started;

/**
 * @brief variable indicating if semaphores & shared memory are set up 
 */
int client_set_up = 0;

/**
 * @brief shm is the structure for the shared memory - it needs to be locked before use with a semaphore
 */
 struct shm_struct *shm;


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

/**
 * @brief executes sem_wait for a specific semaphore
 * @param sem semaphore to execute for
 */
void wait_sem(sem_t *sem);

/**
 * @brief executes sem_post for a specific semaphore
 * @param sem semaphore to execute for
 */
void post_sem(sem_t *sem);


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
    if (client_set_up) {
        /* unmap shared memory */
        if (munmap(shm, sizeof *shm) == -1) {
            printf("could not munmap shared memory");
        }
    }
    if (server != 0) {
        if (sem_close(server) == -1) {
            printf("could not close server semaphore");
        }
    }
    if (client != 0) {
        if (sem_close(client) == -1) {
            printf("could not close client semaphore");
        }
    }
    if (interaction_started != 0) {
        if (sem_close(interaction_started) == -1) {
            printf("could not close interaction_started semaphore");
        }
    }
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

void wait_sem(sem_t *sem) {
    if (sem_wait(sem) == -1) {
        bail_out(errno, "sem_wait failed");
    }
}

void post_sem(sem_t *sem) {
    if (sem_post(sem) == -1) {
        bail_out(errno, "sem_post failed");
    }
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
    int shmfd = shm_open(SHM_SERVER, O_RDWR, PERMISSION);
    if (shmfd == -1) {
        bail_out(errno, "server seems to be down");
    }
    /* set up shared memory for the client to use */
    if (ftruncate(shmfd, sizeof *shm) == -1) {
        bail_out(errno, "could not ftruncate");
    }
    shm = mmap(NULL, sizeof *shm, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shm == MAP_FAILED) {
        bail_out(errno, "could not correctly execute mmap");
    }
    if (close(shmfd) == -1) {
        bail_out(errno, "could not close shm file descriptor");
    }
    /* open semaphores */
    client = sem_open(SEM_CLIENT, 0);
    if (client == SEM_FAILED) {
        bail_out(errno, "could not open client sempahore");
    }
    server = sem_open(SEM_SERVER, 0);
    if (server == SEM_FAILED) {
        bail_out(errno, "could not opne server sempahore");
    }
    interaction_started = sem_open(SEM_INTERACTION_STARTED, 0);
    if (interaction_started == SEM_FAILED) {
        bail_out(errno, "could not open interaction_started sempahore");
    }

    client_set_up = 1;

    /* via stdin get commands from user to send to server */
    /* as soon as client received command it gets sent to the server, proccessed there and the client reads the reply and prints it */
    char* line = malloc((size_t) LINE_SIZE);
    while(fgets(line, LINE_SIZE-1 , stdin) != NULL) {
        if (quit == 1) {
            printf("caught signal - shutting down\n");
            break;
        }
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
            if (pid < 0) {
                print_invalid_command();
                continue;
            }
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

        /* lock the shared memory space for the current client */
        wait_sem(interaction_started);

        /* write the request to the server */
        wait_sem(client);
        /* critical section start */
        shm->pid = pid;
        shm->pid_cmd = pid_cmd;
        shm->info = info;
        /* critical section end */
        post_sem(server);

        /* read the servers response */
        wait_sem(client);
        /* critical section start */
        if (shm->pid_cmd != -1) {
            printf("- %d\n", shm->value_d);
        } else if (shm->info == 3) {
            if (shm->value[(strlen(shm->value)-1)] == '\n') {
                char *pos = shm->value+strlen(shm->value)-1;
                *pos = '\0';
            }
            printf("%d %s\n", shm->pid, shm->value);
        } else {
            printf("%d %d\n", shm->pid, shm->value_d);
        }
        /* critical section end */
        post_sem(server);

        /* unlock the shared memory space */
        post_sem(interaction_started);
    }
    free(line);

    /* EOF file got read - shut down client */
    free_resources();
    return 0;
}