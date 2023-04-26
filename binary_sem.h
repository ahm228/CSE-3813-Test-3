#ifndef BINARY_SEM_H
#define BINARY_SEM_H

int set_semvalue(int sem_id, int sem_num, int val);
void del_semvalue(int sem_id);
int reserve_semaphore(int sem_id, int sem_num);
int release_semaphore(int sem_id, int sem_num);

#endif
