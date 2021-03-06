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
//typedef struct process process;

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
 int server_set_up = 0;

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
 * @brief Signal handler for SIGINT & SIGTERM which should shut down the server
 * @param sig Signal number catched
 */
static void signal_quit_handler(int sig);

/**
 * @brief Signal handler for SIGUSR1 which should print all data in the database
 * @param sig Signal number catched
 */
static void signal_print_db_handler(int sig);

/**
 * @brief this funciton calculates and returns the result of the calculation of min/max/sum/avg over all processes
 * @param command 0 - min, 1 - max, 2 - sum, 3 - avg
 * @param field 0 - cpu, 1 - mem, 2 - time
 * @return returns the result as an integer
 */
static int calculate_min_max_sum_avg(int command, int field);

/**
 * @brief this funciton searches the list of processes and returns the value
 * @param pid for wich to look for
 * @param field 0 - cpu, 1 - mem, 2 - time
 * @return returns the value if it was found - otherwise -1
 */
static int get_cpu_mem_time(int pid, int field);


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
    if (server_set_up) {
        /* unmap shared memory */
        if (munmap(shm, sizeof *shm) == -1) {
            printf("could not munmap shared memory");
        }
        /* remove shared memory */
        if (shm_unlink(SHM_SERVER) == -1) {
            printf("could not unlink shared memory");
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
    if (server_set_up) {
        if (sem_unlink(SEM_SERVER) == -1) {
            printf("could not unlink server sempahore");
        }
        if (sem_unlink(SEM_CLIENT) == -1) {
            printf("could not unlink client sempahore");
        }
        if (sem_unlink(SEM_INTERACTION_STARTED) == -1) {
            printf("could not unlink interaction_started sempahore");
        }
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
                if (endptr == s || strcmp("", endptr) != 0  || ((i == LONG_MAX || i == LONG_MIN) && errno == ERANGE)) {
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
}

static void signal_quit_handler(int sig) {
    quit = 1;
}

static void signal_print_db_handler(int sig) {
    print_db = 1;
}

static int calculate_min_max_sum_avg(int command, int field) {
    int min = INT_MAX;
    int max = INT_MIN;
    int sum = 0;
    int cnt = 0;
    for (int i = 0; i < count_porccesses; ++i) {
        if (field == 0) {
            if (processes[i].p_cpu < min) {
                min = processes[i].p_cpu;
            } if (processes[i].p_cpu > max) {
                max = processes[i].p_cpu;
            }
            sum += processes[i].p_cpu;
        } else if (field == 1) {
            if (processes[i].p_mem < min) {
                min = processes[i].p_mem;
            } if (processes[i].p_mem > max) {
                max = processes[i].p_mem;
            }
            sum += processes[i].p_mem;
        } else if (field == 2) {
            if (processes[i].p_time < min) {
                min = processes[i].p_time;
            } if (processes[i].p_time > max) {
                max = processes[i].p_time;
            }
            sum += processes[i].p_time;
        } else {
            bail_out(EXIT_FAILURE, "wrong input received at server end for calculating min/max/sum/avg - non existing field (cpu/mem/time)");
        }
        ++cnt;
    }
    if (command == 0) {
        return min;
    } else if (command == 1) {
        return max;
    } else if (command == 2) {
        return sum;
    } else if (command == 3) {
        return sum/cnt;
    }
    bail_out(EXIT_FAILURE, "wrong input received at server end for calculating min/max/sum/avg - non existing command (min/max/sum/avg)");
    return 0;
}

static int get_cpu_mem_time(int pid, int field) {
    for (int i = 0; i < count_porccesses; ++i) {
        if (processes[i].pid == pid) {
            if (field == 0) {
                return processes[i].p_cpu;
            } else if (field == 1) {
                return processes[i].p_mem;
            } else if (field == 2) {
                return processes[i].p_time;
            } else {
                if (field == 3) {
                    return -1;
                }
                bail_out(EXIT_FAILURE, "wrong input received at server end for calculating min/max/sum/avg - non existing field (cpu/mem/time)");
            }
        }
    }
    return -1;
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

    /* setup shared memory */
    int shmfd = shm_open(SHM_SERVER, O_RDWR | O_CREAT, PERMISSION);
    if (shmfd == -1) {
        bail_out(errno, "could not set up server shared memory");
    }
    /* adjust the length of the shared memory */
    if (ftruncate(shmfd, sizeof *shm) == -1) {
        bail_out(errno, "could not ftruncate");
    }
    /* establish mapping */
    shm = mmap(NULL, sizeof *shm, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shm == MAP_FAILED) {
        bail_out(errno, "could not correctly execute mmap");
    }
    if (close(shmfd) == -1) {
        bail_out(errno, "could not close shm file descriptor");
    }

    /* set up semaphores */
    client = sem_open(SEM_CLIENT, O_CREAT | O_EXCL, PERMISSION, 0);
    if (client == SEM_FAILED) {
        bail_out(errno, "could not set up client sempahore");
    }
    server = sem_open(SEM_SERVER, O_CREAT | O_EXCL, PERMISSION, 1);
    if (server == SEM_FAILED) {
        bail_out(errno, "could not set up server sempahore");
    }
    interaction_started = sem_open(SEM_INTERACTION_STARTED, O_CREAT | O_EXCL, PERMISSION, 0);
    if (interaction_started == SEM_FAILED) {
        bail_out(errno, "could not set up interaction_started sempahore");
    }

    /* parse arguments */
    parse_args(argc, argv);

    server_set_up = 1;

    /* set shared memory to default values */
    if (sem_wait(server) == -1) {
        bail_out(errno, "sem_wait failed");
    }
    /* critical section start */
    shm->pid = -1;
    shm->pid_cmd = -1;
    shm->info = -1;
    (void)strncpy(shm->value, "no command\0", LINE_SIZE-1);
    shm->value_d = -1;
    /* critical section end */
    if (sem_post(server) == -1) {
        bail_out(errno, "sem_post failed");
    }
    if (sem_post(client) == -1) {
        bail_out(errno, "sem_post failed");
    }
    if (sem_post(interaction_started) == -1) {
        bail_out(errno, "sem_post failed");
    }

    /* wait for requests of clients, and write back answers */
    while (TRUE) {
        if (quit == 1) {
            printf("caught signal - shutting down\n");
            break;
        }
        if (print_db == 1) {
            for (int i = 0; i < count_porccesses; ++i) {
                printf("proccess - pid: %d, cpu: %d, mem: %d, time: %d, command: %s\n", processes[i].pid, processes[i].p_cpu, processes[i].p_mem, processes[i].p_time, processes[i].p_command);
            }
            print_db = 0;
        }
        /* wait for client to connect then read the request and write the response */
        if (sem_wait(server) == -1) {
            if (errno == EINTR) {
                continue;
            }
            bail_out(errno, "sem_wait failed");
        }
        /* critical section start */
        if (shm->pid_cmd != -1) {
            shm->value_d = calculate_min_max_sum_avg(shm->pid_cmd, shm->info);
        } else {
            if (shm->info == 3) {
                for (int i = 0; i < count_porccesses; ++i) {
                    if (processes[i].pid == shm->pid) {
                        memset(&shm->value[0], 0, sizeof(shm->value));
                        (void)strncpy(shm->value, processes[i].p_command, LINE_SIZE-1);
                    }
                }
            } else {
                shm->value_d = get_cpu_mem_time(shm->pid, shm->info);
            }
        }
        /* critical section end */
        if (sem_post(client) == -1) {
            bail_out(errno, "sem_post failed");
        }

        /* clean up as soon as the client finished reading */
        if (sem_wait(server) == -1) {
            if (errno == EINTR) {
                continue;
            }
            bail_out(errno, "sem_wait failed");
        }
        /* critical seciton start */
        shm->pid = -1;
        shm->pid_cmd = -1;
        shm->info = -1;
        (void)strncpy(shm->value, "no command\0", LINE_SIZE-1);
        shm->value_d = -1;
        /* critical section end */
        if (sem_post(client) == -1) {
            bail_out(errno, "sem_post failed");
        }
    }

    free_resources();
    return 0;
}