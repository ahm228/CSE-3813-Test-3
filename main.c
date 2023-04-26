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
    int num_blocks;
    struct {
        int length;
        char character;
    } blocks[20];
} SharedData;

// Binary semaphore functions
int set_semvalue(int sem_id, int sem_num, int val) {
    union semun sem_union;
    sem_union.val = val;
    return semctl(sem_id, sem_num, SETVAL, sem_union);
}

void del_semvalue(int sem_id) {
    union semun sem_union;
    semctl(sem_id, 0, IPC_RMID, sem_union);
}

int reserve_semaphore(int sem_id, int sem_num) {
    struct sembuf sem_b;
    sem_b.sem_num = sem_num;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;
    return semop(sem_id, &sem_b, 1);
}

int release_semaphore(int sem_id, int sem_num) {
    struct sembuf sem_b;
    sem_b.sem_num = sem_num;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;
    return semop(sem_id, &sem_b, 1);
}

void child_process(int sem_id, int shm_id) {
    SharedData *shared_data;
    shared_data = shmat(shm_id, NULL, 0);
    srand(time(NULL) ^ (getpid() << 16));

    reserve_semaphore(sem_id, 0);

    shared_data->num_blocks = (rand() % 11) + 10;

    for (int i = 0; i < shared_data->num_blocks; i++) {
        shared_data->blocks[i].length = (rand() % 9) + 2;
        shared_data->blocks[i].character = 'a' + (rand() % 26);
    }

    release_semaphore(sem_id, 1);

    reserve_semaphore(sem_id, 0);

    shmdt(shared_data);

    release_semaphore(sem_id, 1);
}

void parent_process(int sem_id, int shm_id) {
    SharedData *shared_data;
    shared_data = shmat(shm_id, NULL, 0);
    srand(time(NULL) ^ (getpid() << 16));

    reserve_semaphore(sem_id, 1);

    int width = (rand() % 6) + 10;
    int count = 0;

    for (int i = 0; i < shared_data->num_blocks; i++) {
        for (int j = 0; j < shared_data->blocks[i].length; j++) {
            printf("%c", shared_data->blocks[i].character);
            count++;

            if (count == width) {
                printf("\n");
                count = 0;
            }
        }
    }

    release_semaphore(sem_id, 0);

    reserve_semaphore(sem_id, 1);

    shmdt(shared_data);
}

int main(int argc, char *argv[]) {
    int sem_id, shm_id;
    key_t ipc_private_key = IPC_PRIVATE;

    sem_id = semget(ipc_private_key, 2, 0666 | IPC_CREAT);
    set_semvalue(sem_id, 0, 1);
    set_semvalue(sem_id, 1, 0);

    shm_id = shmget(ipc_private_key, sizeof(SharedData), 0666 | IPC_CREAT);

    pid_t pid = fork();

    if (pid == 0) {
        child_process(sem_id, shm_id);
    } 
    else {
        parent_process(sem_id, shm_id);
        wait(NULL);

        del_semvalue(sem_id);
        semctl(sem_id, 0, IPC_RMID, NULL);

        shmctl(shm_id, IPC_RMID, NULL);
    }

    exit(EXIT_SUCCESS);
}
