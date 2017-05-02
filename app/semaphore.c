#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

union semun
{
    int                 val;
    struct semid_ds     *buf;
    unsigned short int  *array;
    struct seminfo      *__buf;
};

static int pv(int sem_id, int op)
{
    struct sembuf   sem_b;
    sem_b.sem_num   = 0;
    sem_b.sem_op    = op;
    sem_b.sem_flg   = SEM_UNDO;
    return semop(sem_id, &sem_b, 1);
}

static int set_semvalue(int sem_id)
{
    union semun sem_union;

    sem_union.val = 1;
    if (semctl(sem_id, 0, SETVAL, sem_union) == -1) {
        return -1;
    }

    return 0;
}

int semaphore_p(int sem_id)
{
    return pv(sem_id, -1);
}

int semaphore_v(int sem_id)
{
    return pv(sem_id, 1);
}

int semaphore_create(int key)
{
    int ret = semget((key_t)key, 1, IPC_EXCL | IPC_CREAT);
    if (ret < 0) {
        if (errno == EEXIST) { 
            ret = semget((key_t)key, 1, 0666 | IPC_CREAT);
        }
    } else {
        set_semvalue(ret);
    }

    return ret;
}

void semaphore_destory(int sem_id)
{
    union semun sem_union;
    semctl(sem_id, 0, IPC_RMID, sem_union);
}
