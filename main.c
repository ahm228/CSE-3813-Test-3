#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
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

void childProcess(int semID, int shmID) {
    SharedData *sharedData;
    sharedData = shmat(shmID, NULL, 0);
    srand(time(NULL) ^ (getpid() << 16));

    reserveSemaphore(semID, 0);

    sharedData->numBlocks = (rand() % 11) + 10;

    for (int i = 0; i < sharedData->numBlocks; i++) {
        sharedData->blocks[i].length = (rand() % 9) + 2;
        sharedData->blocks[i].character = 'a' + (rand() % 26);
    }

    releaseSemaphore(semID, 1);

    reserveSemaphore(semID, 0);

    shmdt(sharedData);

    releaseSemaphore(semID, 1);
}

void parentProcess(int semID, int shmID) {
    SharedData *sharedData;
    sharedData = shmat(shmID, NULL, 0);
    srand(time(NULL) ^ (getpid() << 16));

    reserveSemaphore(semID, 1);

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

    releaseSemaphore(semID, 0);

    reserveSemaphore(semID, 1);

    shmdt(sharedData);
}

int main(int argc, char *argv[]) {
    int semID, shmID;
    key_t key = IPC_PRIVATE;

    semID = semget(key, 2, 0666 | IPC_CREAT);
    setSemValue(semID, 0, 1);
    setSemValue(semID, 1, 0);

    shmID = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);

    pid_t pid = fork();

    if (pid == 0) {
        childProcess(semID, shmID);
    } 
    else {
        parentProcess(semID, shmID);
        wait(NULL);

        delSemValue(semID);
        semctl(semID, 0, IPC_RMID, NULL);

        shmctl(shmID, IPC_RMID, NULL);
    }

    exit(EXIT_SUCCESS);
}
