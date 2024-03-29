#ifndef BINARY_SEM_H
#define BINARY_SEM_H

// Binary semaphore function prototypes
int setSemValue(int semID, int semNum, int val);
int delSemValue(int semID);
int reserveSemaphore(int semID, int semNum);
int releaseSemaphore(int semID, int semNum);

#endif
