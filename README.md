# procDB
An university assignment for operating systems in C. It is an process-database implemented as a client-server structure where information exchange works via shared memory.

## Semaphores
Semaphores are a way of locking a specific part of memory so that not more than one process can access that part of memory at once - otherwise they could be writing at the same time or one writing and another reading which would all most likely end in undefined behaviour. It is possible to regulate in wich order the processes can access the memory.

A semaphore is basically a counter. When the value is larger than 0 the process may enter the critical section - the semaphore reduces the value by one. When the value is 0 (or lower - there are different implementations) the process has to wait.

To set up a semaphore use `sem_open`.
```
#define PERMISSION (0600)

client = sem_open(SEM_LOCATION, O_CREAT | O_EXCL, PERMISSION, 0);
if (client == SEM_FAILED)
```
In this case `SEM_LOCATION` is the name of the semaphore.
`O_CREAT` and `O_EXCL` specify that an error should get returned if it already exists.
`PERMISSIONS` in this case is 600 wich means that the owner can read & write.
The last argument specifies the initial value of the semaphore.

If a process wants to enter the critical section it has to call `sem_wait(sem)`. If `sem_wait(sem) == -1` an error occured. It will decrement the value of the semaphore `sem` if the value is greater than 0. Otherwise it will wait until the value of the semaphore is greater than 0.

If a process wants to show that it has left the critical section it has to call `sem_post(sem)` wich will increment the value of the semaphore.

## Shared Memory
