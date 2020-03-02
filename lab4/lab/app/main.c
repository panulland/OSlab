#include "lib.h"
#include "types.h"

int data = 0;

void producer(int pid, sem_t mutex, sem_t empty, sem_t full) {
    int i = pid - 1;
    for(int k = 0; k < 8; k++) {
        sem_wait(&empty);
        printf("pid %d, producer %d, produce, product %d\n", pid, i, k + 1);
        sleep(128);
        printf("pid %d, producer %d, try lock, product %d\n", pid, i, k + 1);
        sem_wait(&mutex);
        printf("pid %d, producer %d, locked\n", pid, i);
        sem_post(&mutex);
        printf("pid %d, producer %d, unlock\n", pid, i);
        sem_post(&full);
    }
}

void consumer(int pid, sem_t mutex, sem_t empty, sem_t full) {
    int i = pid - 3;
    for(int k = 0; k < 4; k++) {
        printf("pid %d, consumer %d, try consume, product %d\n", pid, i, k + 1);
        sem_wait(&full);
        printf("pid %d, consumer %d, try lock, product %d\n", pid, i, k + 1);
        sem_wait(&mutex);
        printf("pid %d, consumer %d, locked\n", pid, i);
        sem_post(&mutex);
        printf("pid %d, consumer %d, unlock\n", pid, i);
        sem_post(&empty);
        sleep(128);
        printf("pid %d, consumer %d, consumed, product %d\n", pid, i, k + 1);
    }
}

int uEntry(void) {
    
    sem_t mutex, empty, full;
    sem_init(&mutex, 1);
    sem_init(&empty, 16);
    sem_init(&full, 0);
    pid_t pid;
    for(int i = 0; i < 6; i++) {
        pid = fork();
        if(pid == 0 || pid == -1)
            break;
    }
    if(pid != -1) {
        pid = getpid();
        if(pid > 3) {
            consumer(pid, mutex, empty, full);
        }
        else if(pid > 1) {
            producer(pid, mutex, empty, full);
        }
        exit();
    }

    /*int i = 4;
    int ret = 0;
    sem_t sem;
    printf("Father Process: Semaphore Initializing.\n");
    ret = sem_init(&sem, 2);
    if (ret == -1) {
        printf("Father Process: Semaphore Initializing Failed.\n");
        exit();
    }

    ret = fork();
    if (ret == 0) {
        while( i != 0) {
            i --;
            printf("Child Process: Semaphore Waiting.\n");
            sem_wait(&sem);
            printf("Child Process: In Critical Area.\n");
        }
        printf("Child Process: Semaphore Destroying.\n");
        sem_destroy(&sem);
        exit();
    }
    else if (ret != -1) {
        while( i != 0) {
            i --;
            printf("Father Process: Sleeping.\n");
            sleep(128);
            printf("Father Process: Semaphore Posting.\n");
            sem_post(&sem);
        }
        printf("Father Process: Semaphore Destroying.\n");
        sem_destroy(&sem);
        exit();
    }*/
    
    return 0;
}
