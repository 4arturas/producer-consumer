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

void cleanup(key_t shm_key, int semid) {
    int shmid = shmget(shm_key, 0, 0);
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
        printf("Shared memory removed.\n");
    } else {
        perror("shmget");
    }

    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
        printf("Semaphore removed.\n");
    }
}

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
    shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
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

    *count = 0;

    // while (1) 
    for ( int i = 0; i < 5; i++ ) 
    {
        put.sem_num = 0; // Wait for the producer semaphore
        put.sem_op = -1; // Decrement the semaphore value
        put.sem_flg = 0;

        if (semop(semid, &put, 1) == -1) {
            perror("semop");
            exit(EXIT_FAILURE);
        }

        (*count)++;
        printf("Produced item: %d\n", *count);

        get.sem_num = 1; // Signal the consumer semaphore
        get.sem_op = 1; // Increment the semaphore value

        if (semop(semid, &get, 1) == -1) {
            perror("semop");
            exit(EXIT_FAILURE);
        }

        sleep(1);
    }

    shmdt(count);
    cleanup(key, semid);

    return 0;
}