#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "binary_sem.h"
#include "semun.h"

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
