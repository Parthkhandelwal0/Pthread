// File: mypthread_t.h

// List all group members' names: Parth Khandelwal (pk684), Lakshit Pant (lp749)
// iLab machine tested on: ilab1

#ifndef MYTHREAD_T_H
#define MYTHREAD_T_H

#define _GNU_SOURCE

/* in order to use the built-in Linux pthread library as a control for benchmarking, you have to comment the USE_MYTHREAD macro */
//#define USE_MYTHREAD 1

#define QUANTUMTIME 30

#define READY 0
#define SCHEDULED 1
#define BLOCKED 2

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

typedef uint mypthread_t;

	/* add important states in a thread control block */
typedef struct threadControlBlock{

	// thread Id
	mypthread_t threadId;

	// thread context
	ucontext_t ctx;

	// thread status
	int status;
	
	//thread priority
	int priority;

} tcb;

typedef struct mypthread{

	tcb * threadControlBlock;

} mypthread;

/* mutex struct definition */
typedef struct mypthread_mutex_t
{
	
	//flag is used to indicate whether mutex is available or nor
	int flag;

	//pointer to the thread holding the mutex
	mypthread * mthread;

} mypthread_mutex_t;

typedef struct node{

	mypthread * t;
	struct node * next;

} node;

//creating queue struct
typedef struct runqueue{

	//head
	node *head;
	//tail
	node *tail;

} runqueue;

/* Function Declarations: */

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* current thread voluntarily surrenders its remaining runtime for other threads to use */
int mypthread_yield();

/* terminate a thread */
void mypthread_exit(void *value_ptr);

/* wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr);

/* initialize a mutex */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire a mutex (lock) */
int mypthread_mutex_lock(mypthread_mutex_t *mutex);

/* release a mutex (unlock) */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex);

/* destroy a mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex);

#ifdef USE_MYTHREAD
#define pthread_t mypthread_t
#define pthread_mutex_t mypthread_mutex_t
#define pthread_create mypthread_create
#define pthread_exit mypthread_exit
#define pthread_join mypthread_join
#define pthread_mutex_init mypthread_mutex_init
#define pthread_mutex_lock mypthread_mutex_lock
#define pthread_mutex_unlock mypthread_mutex_unlock
#define pthread_mutex_destroy mypthread_mutex_destroy
#endif

#endif
