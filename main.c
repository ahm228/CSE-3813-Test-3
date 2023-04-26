#include <stdio.h>
#include <stdlib.h>
#include <unistd.>
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

    if (reserveSemaphore(semID, 0) == -1) {
        perror("reserveSemaphore");
        exit(EXIT_FAILURE);
    }

    sharedData->numBlocks = (rand() % 11) + 10;

    for (int i = 0; i < sharedData->numBlocks; i++) {
        sharedData->blocks[i].length = (rand() % 9) + 2;
        sharedData->blocks[i].character = 'a' + (rand() % 26);
    }

    if (releaseSemaphore(semID, 1) == -1) {
        perror("releaseSemaphore");
        exit(EXIT_FAILURE);
    }

    if (reserveSemaphore(semID, 0) == -1) {
        perror("reserveSemaphore");
        exit(EXIT_FAILURE);
    }

     if (shmdt(sharedData) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

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

    if (reserveSemaphore(semID, 1) == -1) {
        perror("reserveSemaphore");
        exit(EXIT_FAILURE);
    }

    int width = (rand() % 6) + 10;
    int count = 0;

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

    if (releaseSemaphore(semID, 0) == -1) {
    perror("releaseSemaphore");
    exit(EXIT_FAILURE);
    }

    if (reserveSemaphore(semID, 1) == -1) {
    perror("reserveSemaphore");
    exit(EXIT_FAILURE);
    }

    if (shmdt(sharedData) == -1) {
    perror("shmdt");
    exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    int semID, shmID;
    key_t key = IPC_PRIVATE;

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

    if (pid == 0) {
        childProcess(semID, shmID);
    } 
    
    else {
        parentProcess(semID, shmID);
        wait(NULL);

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
