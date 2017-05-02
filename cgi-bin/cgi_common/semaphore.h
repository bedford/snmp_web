#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#define RS232_KEY   (1234)
#define RS485_KEY   (1235)
#define DI_KEY      (1236)

int semaphore_p(int sem_id);

int semaphore_v(int sem_id);

int semaphore_create(int key);

void semaphore_destory(int sem_id);

#endif
