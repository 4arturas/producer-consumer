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
#define SHM_SIZE sizeof(int)   // Size of shared memory segment

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
    shmid = shmget(key, SHM_SIZE, 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach to shared memory
    int *count = (int *)shmat(shmid, NULL, 0);
    if (count == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    while ( 1 )
//    for (int i = 0; i < 5; i++)
    {
        get.sem_num = 1; // Wait for the consumer semaphore
        get.sem_op = -1; // Decrement the semaphore value
        get.sem_flg = 0;

        if (semop(semid, &get, 1) == -1) {
            perror("semop");
            exit(EXIT_FAILURE);
        }

        if (*count == -1) { // Check for termination signal
            break;
        }

        printf("Consumed item: %d\n", *count);

        put.sem_num = 0; // Signal the producer semaphore
        put.sem_op = 1; // Increment the semaphore value

        if (semop(semid, &put, 1) == -1) {
            perror("semop");
            exit(EXIT_FAILURE);
        }
    }

    shmdt(count); // Detach from shared memory

    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    printf("Consumer done!\n");

    return 0;
}