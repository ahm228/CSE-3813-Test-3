#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include <errno.h>
#include "semun.h"
#include "binary_sem.h"

typedef struct {
    int numBlocks;
    struct {
        int length;
        char character;
    } blocks[20];
} SharedData;

// Binary semaphore functions
int setSemValue(int semID, int semNum, int val) {
    union semun semUnion;
    semUnion.val = val;
    return semctl(semID, semNum, SETVAL, semUnion);
}

void delSemValue(int semID) {
    union semun semUnion;
    semctl(semID, 0, IPC_RMID, semUnion);
}

int reserveSemaphore(int semID, int semNum) {
    struct sembuf semB;
    semB.semNum = semNum;
    semB.semOp = -1;
    semB.semFlg = SEM_UNDO;
    return semop(semID, &semB, 1);
}

int releaseSemaphore(int semID, int semNum) {
    struct sembuf semB;
    semB.semNum = semNum;
    semB.semOp = 1;
    semB.semFlg = SEM_UNDO;
    return semop(semID, &semB, 1);
}

// Function for child process to generate blocks
void childProcess(int semID, int shmID) {
    SharedData *sharedData;
    sharedData = shmat(shmID, NULL, 0);
    if (sharedData == (void*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) ^ (getpid() << 16));

    // Reserve the first semaphore to prevent the parent from accessing shared memory
    if (reserveSemaphore(semID, 0) == -1) {
        perror("reserveSemaphore");
        exit(EXIT_FAILURE);
    }

    // Generate a random number of blocks to generate
    sharedData->numBlocks = (rand() % 11) + 10;

    // Generate random data for each block
    for (int i = 0; i < sharedData->numBlocks; i++) {
        sharedData->blocks[i].length = (rand() % 9) + 2;
        sharedData->blocks[i].character = 'a' + (rand() % 26);
    }

    // Release the first semaphore to allow the parent to access shared memory
    if (releaseSemaphore(semID, 1) == -1) {
        perror("releaseSemaphore");
        exit(EXIT_FAILURE);
    }

    // Wait for the parent to finish accessing shared memory
    if (reserveSemaphore(semID, 0) == -1) {
        perror("reserveSemaphore");
        exit(EXIT_FAILURE);
    }

    // Detach from shared memory
    if (shmdt(sharedData) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    // Release the second semaphore to allow the parent to continue
    if (releaseSemaphore(semID, 1) == -1) {
        perror("releaseSemaphore");
        exit(EXIT_FAILURE);
    }
}

// Function for parent process to print blocks
void parentProcess(int semID, int shmID) {
    SharedData *sharedData;
    sharedData = shmat(shmID, NULL, 0);
    if (sharedData == (void*) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL) ^ (getpid() << 16));

    // Wait for the child to finish generating blocks
    if (reserveSemaphore(semID, 1) == -1) {
        perror("reserveSemaphore");
        exit(EXIT_FAILURE);
    }

    // Generate a random width for the output block
    int width = (rand() % 6) + 10;
    int count = 0;

    // Print the blocks
    for (int i = 0; i < sharedData->numBlocks; i++) {
        for (int j = 0; j < sharedData->blocks[i].length; j++) {
            printf("%c", sharedData->blocks[i].character);
            count++;

            if (count == width) {
                printf("\n");
                count = 0;
            }
        }
    }

    // Release the first semaphore to allow the child to continue
    if (releaseSemaphore(semID, 0) == -1) {
    perror("releaseSemaphore");
    exit(EXIT_FAILURE);
    }

    // Wait for the child to finish detaching from shared memory
    if (reserveSemaphore(semID, 1) == -1) {
    perror("reserveSemaphore");
    exit(EXIT_FAILURE);
    }

    // Detach from shared memory
    if (shmdt(sharedData) == -1) {
    perror("shmdt");
    exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    int semID, shmID;
    key_t key = IPC_PRIVATE;

    // Create the semaphore set
    semID = semget(key, 2, 0666 | IPC_CREAT);
    if (semID == -1) {
    perror("semget");
    exit(EXIT_FAILURE);
    }

    if (setSemValue(semID, 0, 1) == -1) {
    perror("setSemValue");
    exit(EXIT_FAILURE);
    }
    
    if (setSemValue(semID, 1, 0) == -1) {
    perror("setSemValue");
    exit(EXIT_FAILURE);
    }

    // Create the shared memory segment
    shmID = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);
    if (shmID == -1) {
    perror("shmget");
    exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
    }

    // If the process is the child, generate the blocks
    if (pid == 0) {
        childProcess(semID, shmID);
    } 
    
    // If the process is the parent, print the blocks and wait for the child to finish
    else {
        parentProcess(semID, shmID);
        wait(NULL);

        // Release the semaphores and shared memory segment
        if (delSemValue(semID) == -1) {
        perror("delSemValue");
        exit(EXIT_FAILURE);
        }
        
        if (semctl(semID, 0, IPC_RMID, NULL) == -1) {
        perror("semctl");
        exit(EXIT_FAILURE);
        }
        
        if (shmctl(shmID, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}
