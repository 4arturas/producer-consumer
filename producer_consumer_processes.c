#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <errno.h>

#define PATHNAME "pathname.txt" // Path to an existing file for ftok
#define PROJECT_ID 'A'         // Project identifier
#define BUFFER_SIZE 5  // Size of the buffer

// Semaphore operations
void sem_wait(int semid) {
    struct sembuf sb = {0, -1, 0}; // Decrement semaphore
    semop(semid, &sb, 1);
}

void sem_signal(int semid) {
    struct sembuf sb = {0, 1, 0}; // Increment semaphore
    semop(semid, &sb, 1);
}

int main() {
    int shmid;
    int semid;
    int *buffer;

    // Generate unique key for shared memory and semaphore
    key_t key = ftok( PATHNAME, PROJECT_ID ); // Create a unique key based on a file
    if (key == -1) {
        perror("ftok failed");
        exit(EXIT_FAILURE);
    }

    // Create shared memory segment
    shmid = shmget(key, sizeof(int) * BUFFER_SIZE + sizeof(int) * 2, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    buffer = (int *)shmat(shmid, NULL, 0);
    if (buffer == (void *)-1) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    // Initialize buffer and counters
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = 0; // Initialize buffer
    }

    int in = 0; // Index for producer
    int out = 0; // Index for consumer

    // Create semaphore set with two semaphores: one for empty slots and one for full slots
    semid = semget(key, 2, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores: empty slots and full slots
    semctl(semid, 0, SETVAL, BUFFER_SIZE); // Semaphore for empty slots
    semctl(semid, 1, SETVAL, 0);           // Semaphore for full slots

    if (fork() == 0) { // Producer process
        for (int i = 0; i < 10; i++) {
            sem_wait(semid);       // Wait if there are no empty slots

            buffer[in % BUFFER_SIZE] = i; // Produce an item
            printf("Produced: %d\n", i);
            in++;

            sem_signal(semid + 1); // Signal that an item has been produced (increment full slots)
//            sleep(1);              // Simulate time taken to produce an item
        }
        exit(0);
    } else { // Consumer process
        for (int i = 0; i < 10; i++) {
            sem_wait(semid + 1);   // Wait if there are no full slots

            int item = buffer[out % BUFFER_SIZE]; // Consume an item
            printf("Consumed: %d\n", item);
            out++;

            sem_signal(semid);      // Signal that an item has been consumed (increment empty slots)
//            sleep(2);               // Simulate time taken to consume an item
        }

        wait(NULL);                // Wait for producer to finish

        shmdt(buffer);             // Detach shared memory
        shmctl(shmid, IPC_RMID, NULL); // Remove shared memory segment
        semctl(semid, 0, IPC_RMID);     // Remove semaphore set
    }

    return 0;
}