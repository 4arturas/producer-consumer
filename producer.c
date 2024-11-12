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
    int nsems = 2;
    int semid, shmid;
    struct sembuf put, get;
    unsigned short init_values[2] = {1, 0}; // Initial values for semaphores

    key_t key = ftok(PATHNAME, PROJECT_ID);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Create semaphore
    semid = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    union semun u;
    u.array = init_values;
    if (semctl(semid, 0, SETALL, u) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
    }

    // Create shared memory
    shmid = shmget(key, SHM_SIZE * sizeof(int), IPC_CREAT | 0666);
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

    // Print PID of the producer
    printf("Producer PID: %d\n", getpid());

    // Producer Logic
    for (int i = 0; i < 5; i++) {
        put.sem_num = 0; // Wait for the producer semaphore
        put.sem_op = -1; // Decrement the semaphore value
        put.sem_flg = 0;

        if (semop(semid, &put, 1) == -1) {
            perror("semop");
            exit(EXIT_FAILURE);
        }

        // Produce an item and store it in the buffer
        buffer[i % 3] = i + 1; // Store produced item in a cyclic manner
//        printf("Produced item: %d at index %d\n", buffer[i % 3], i % 3);

        get.sem_num = 1; // Signal the consumer semaphore
        get.sem_op = 1; // Increment the semaphore value

        if (semop(semid, &get, 1) == -1) {
            perror("semop");
            exit(EXIT_FAILURE);
        }

        sleep(1); // Simulate time taken to produce an item
    }

    // Signal completion by setting a special value (-1)
    for (int j = 0; j < 3; j++) {
        buffer[j] = -1; // Indicate no more items will be produced
    }

    get.sem_num = 1; // Signal the consumer semaphore one last time
    get.sem_op = 3; // Increment the semaphore value by 3 to wake all consumers

    if (semop(semid, &get, 1) == -1) {
        perror("semop");
        exit(EXIT_FAILURE);
    }

    shmdt(buffer); // Detach from shared memory

    // Cleanup resources directly in main function
    shmctl(shmid, IPC_RMID, NULL); // Remove shared memory segment
    semctl(semid, 0, IPC_RMID);     // Remove semaphore set

    printf("Producer done!\n");

    return 0;
}