#include "pthread.h"
#include "lib.h"
/*
 * pthread lib here
 * 用户态多线程写在这
 */
 
ThreadTable tcb[MAX_TCB_NUM];
int current;

#define getRegs(edi, esi, ebp, esp, ebx, edx, ecx, eax, eip) \
    asm volatile("movl %%edi, %0": "=m" (edi)); \
    asm volatile("movl %%esi, %0": "=m" (esi)); \
    asm volatile("movl %%ebx, %0": "=m" (ebx)); \
    asm volatile("movl %%edx, %0": "=m" (edx)); \
    asm volatile("movl %%ecx, %0": "=m" (ecx)); \
    asm volatile("movl %%eax, %0": "=m" (eax)); \
    asm volatile("movl (%%ebp), %0": "=r" (ebp)); \
    asm volatile("movl %%esp, %0": "=m" (esp)); \
    esp += 8; \
    asm volatile("movl 0x4(%%ebp), %0": "=r" (eip)); 

#define saveTCBcont(edi, esi, ebp, esp, ebx, edx, ecx, eax, eip) \
    tcb[current].cont.edi = edi; \
    tcb[current].cont.esi = esi; \
    tcb[current].cont.ebp = ebp; \
    tcb[current].cont.esp = esp; \
    tcb[current].cont.ebx = ebx; \
    tcb[current].cont.edx = edx; \
    tcb[current].cont.ecx = ecx; \
    tcb[current].cont.eax = eax; \
    tcb[current].cont.eip = eip;

void recover(uint32_t edi, uint32_t esi, uint32_t ebp, uint32_t esp, uint32_t ebx, uint32_t edx, uint32_t ecx, uint32_t eax, uint32_t eip) {
    asm volatile("movl %0, %%edi":: "m" (edi));
    asm volatile("movl %0, %%esi":: "m" (esi));
    asm volatile("movl %0, %%ebx":: "m" (ebx));
    asm volatile("movl %0, %%edx":: "m" (edx));
    asm volatile("movl %0, %%ecx":: "m" (ecx));
    asm volatile("movl %0, %%eax":: "m" (eax));
    asm volatile("movl %0, %%esp":: "m" (esp));
    asm volatile("movl %0, %%eax":: "m" (eip));
    asm volatile("movl %0, %%ebp":: "m" (ebp));
    asm volatile("jmp %eax");
}

void pthread_initial(void){
    int i;
    for (i = 0; i < MAX_TCB_NUM; i++) {
        tcb[i].state = STATE_DEAD;
        tcb[i].joinid = -1;
    }
    tcb[0].state = STATE_RUNNING;
    tcb[0].pthid = 0;
    current = 0; // main thread
    return;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg){
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax, eip;
    getRegs(edi, esi, ebp, esp, ebx, edx, ecx, eax, eip);
    int i;
    for(i = 0; i < MAX_TCB_NUM; i++) {
        if(tcb[i].state == STATE_DEAD)
            break;
    }
    if(i != MAX_TCB_NUM) {
        *thread = (pthread_t)i;
        tcb[i].state = STATE_RUNNING;
        tcb[i].pthid = i;
        tcb[i].pthArg = (uint32_t)arg;
        tcb[i].cont.eip = (uint32_t)start_routine;
        tcb[i].cont.ebp = (uint32_t)(tcb[i].stack + MAX_STACK_SIZE -1);
        tcb[i].cont.esp = tcb[i].cont.ebp;
        tcb[i].stackTop = 0;
        tcb[current].state = STATE_RUNNABLE;
        saveTCBcont(edi, esi, ebp, esp, ebx, edx, ecx, eax, eip)
        current = i;
        asm volatile("movl %0, %%ebp":: "r" (tcb[current].cont.ebp));
        asm volatile("movl %ebp, %esp");
        asm volatile("pushl %0":: "m" (tcb[current].pthArg));
        asm volatile("call %0":: "r" (tcb[current].cont.eip));
        return 0;
    }
    else 
        return -1;
    
}

void pthread_exit(void *retval){
    tcb[current].state = STATE_DEAD;
    int i = (current + 1) % MAX_TCB_NUM;
	int j;
	for(j = 0; j < MAX_TCB_NUM; j++) {
		if(tcb[(i+j)%MAX_TCB_NUM].state == STATE_RUNNABLE)
			break;
	}
	current = (i + j) % MAX_TCB_NUM;
    tcb[current].state = STATE_RUNNING;
    recover(tcb[current].cont.edi,
            tcb[current].cont.esi,
            tcb[current].cont.ebp,
            tcb[current].cont.esp,
            tcb[current].cont.ebx,
            tcb[current].cont.edx,
            tcb[current].cont.ecx,
            tcb[current].cont.eax,
            tcb[current].cont.eip);
    return;
}

int pthread_join(pthread_t thread, void **retval){
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax, eip;
    getRegs(edi, esi, ebp, esp, ebx, edx, ecx, eax, eip);
    if(tcb[thread].state == STATE_RUNNABLE) {
        tcb[current].state = STATE_BLOCKED;
        saveTCBcont(edi, esi, ebp, esp, ebx, edx, ecx, eax, eip)
        current = thread;
        tcb[current].state = STATE_RUNNING;
        recover(tcb[current].cont.edi,
            tcb[current].cont.esi,
            tcb[current].cont.ebp,
            tcb[current].cont.esp,
            tcb[current].cont.ebx,
            tcb[current].cont.edx,
            tcb[current].cont.ecx,
            tcb[current].cont.eax,
            tcb[current].cont.eip);
        return 0;
    }
    else
        return -1;
}

int pthread_yield(void){
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax, eip;
    getRegs(edi, esi, ebp, esp, ebx, edx, ecx, eax, eip);
    tcb[current].state = STATE_RUNNABLE;
    saveTCBcont(edi, esi, ebp, esp, ebx, edx, ecx, eax, eip)
    int i = (current + 1) % MAX_TCB_NUM;
	int j;
	for(j = 0; j < MAX_TCB_NUM; j++) {
		if(tcb[(i+j)%MAX_TCB_NUM].state == STATE_RUNNABLE)
			break;
	}
	current = (i + j) % MAX_TCB_NUM;
    tcb[current].state = STATE_RUNNING;
    recover(tcb[current].cont.edi,
            tcb[current].cont.esi,
            tcb[current].cont.ebp,
            tcb[current].cont.esp,
            tcb[current].cont.ebx,
            tcb[current].cont.edx,
            tcb[current].cont.ecx,
            tcb[current].cont.eax,
            tcb[current].cont.eip);
    return 0;
}