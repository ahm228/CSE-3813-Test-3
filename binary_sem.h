#ifndef BINARY_SEM_H
#define BINARY_SEM_H

int setSemValue(int semID, int semNum, int val);
void delSemValue(int semID);
int reserveSemaphore(int semID, int semNum);
int releaseSemaphore(int semID, int semNum);

#endif
