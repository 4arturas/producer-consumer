#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#define PATHNAME "pathname.txt" // Path to an existing file for ftok
#define PROJECT_ID 'A'         // Project identifier
#define SHM_SIZE 12            // Size of shared memory segment (for 3 consumers)

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

int main() {
    int semid, shmid;
    struct sembuf put, get;

    key_t key = ftok(PATHNAME, PROJECT_ID);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Get semaphore ID
    semid = semget(key, 0, 0);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // Access shared memory
    shmid = shmget(key, SHM_SIZE * sizeof(int), 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach to shared memory
    int *buffer = (int *)shmat(shmid, NULL, 0);
    if (buffer == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // Print PID of the consumer
    printf("Consumer PID: %d\n", getpid());

    int index = getpid() % 3; // Assign each consumer a specific index based on its PID

    // Consumer Logic
    while (1) {
        get.sem_num = 1; // Wait for the consumer semaphore
        get.sem_op = -1; // Decrement the semaphore value
        get.sem_flg = 0;

        if (semop(semid, &get, 1) == -1) {
            perror("semop");
            exit(EXIT_FAILURE);
        }

        if (buffer[index] == -1) { // Check for termination signal
            break;          // Exit loop if no more items will be produced
        }

        printf("Consumed item: %d by PID: %d from index: %d\n", buffer[index], getpid(), index);

        put.sem_num = 0; // Signal the producer semaphore
        put.sem_op = 1; // Increment the semaphore value

        if (semop(semid, &put, 1) == -1) {
            perror("semop");
            exit(EXIT_FAILURE);
        }
    }

    shmdt(buffer); // Detach from shared memory

    printf("Consumer done!\n");

    return 0;
}