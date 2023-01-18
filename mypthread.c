// File: mypthread.c

// List all group members' names: Parth Khandelwal (pk684), Lakshit Pant (lp749)
// iLab machine tested on: ilab1

#include "mypthread.h"

#define STACK_SIZE SIGSTKSZ

//Structure to compute timestamp of day
struct timeval tv;

// to assign unique ID values to each thread 
mypthread_t thrIdValue = 1;

//levels of mlfq queue (lvl 1 = least priority)
struct runqueue * mlfq_level1; 
struct runqueue * mlfq_level2;
struct runqueue * mlfq_level3; 

//highest priority case (level 4)
struct runqueue * runningQueue;

//current thread
mypthread * currentThread = NULL;

//queue for all blocked threads
struct runqueue * blockedThreads = NULL;

//variable informing scheduler if the current thread has been blocked or not
int isThreadBlocked = 0;

//variable informing scheduler if the current thread yielded within its time slice
int threadYield = 0;

//main thread
int mainThrdstarted = 0;

//context of the scheduler
ucontext_t schedulerContext;
//thread of the scheduler
mypthread *schedulerThread;

//signal handler
struct sigaction sa;

//initializing timer
struct itimerval timer;

//Arrays to store the terminated threads and return values of all the threads
void * thread_returnValues[150];
int threadsTerminated[150];

//Arrival time of each thread
unsigned long threadArrivalTime[150];

//Schedule time for each thread
unsigned long threadScheduleTime[150];

//Completion time for each thread
unsigned long threadCompletionTime[150];

void freeCurrentThread(mypthread * thread);
static void sched_RR(runqueue * queue);
static void sched_MLFQ();
static void schedule();

// Function to add all jobs in runqueue
void addQueue(mypthread * newThread, struct runqueue * queue){

	if((queue -> tail) != NULL){
		//there is at least one thread in the run queue
		node *temp  = (node *)malloc(sizeof (node));
       	temp -> t = newThread;
		temp -> next = NULL;
		//Assign old tail point to new address of new thread 
		queue -> tail -> next = temp;
		// Updating tail to new thread
       	queue -> tail = temp;	


	}else{

		// for empty queue
		node * temp = (node *)malloc(sizeof (node));
		temp -> t = newThread;
		temp -> next = NULL;
		//both head and tail to same thread
		queue -> tail = temp;
	    queue -> head = temp;	

	}

	gettimeofday(&tv, NULL);
	unsigned long time = 1000000 * tv.tv_sec + tv.tv_usec;

	threadArrivalTime[(newThread -> threadControlBlock) -> threadId] = time;
}

//Function to release all threads in block list and add in run queue
void releaseThreads() {
	//each thread from the blocked list is added to runqueue, based on priority
	//nodes in the blocked list freed
	node *curr = blockedThreads -> head;
	node *prev;
	prev = curr;
	while (curr != NULL){

		curr->t->threadControlBlock->status = READY;
		#ifndef MLFQ

			addQueue(curr -> t, runningQueue);

		#else

			int threadPriority = curr -> t -> threadControlBlock -> priority;
					
			if(threadPriority == 4){
				addQueue(curr->t, runningQueue);
			}else if (threadPriority == 3){
				addQueue(curr->t, mlfq_level3);
			}else if (threadPriority == 2){
				addQueue(curr->t, mlfq_level2);
			}else{
				addQueue(curr-> t , mlfq_level1);
			}	
		#endif

		curr = curr -> next;
		//free the memory for that job as it is moved to run queue 
		free(prev);
		prev = curr;

	}

	//all jobs have been moved so head and tail nulled
	blockedThreads -> head = NULL;
	blockedThreads -> tail = NULL;
}


void timerHandler(int signum){
	//On timer trigger swap context to scheduler context
	swapcontext(&((currentThread -> threadControlBlock)->ctx), &schedulerContext);

}

bool isInitialized = false;

ucontext_t mainContext;
mypthread * mainThread;


void initialize()
{
	if (isInitialized == false){

		isInitialized = true;

		//creating scheduling queue for the jobs
		runningQueue = (runqueue *)malloc(sizeof(runqueue)); //level 4

		// creating queue for the MLFQ levels
		mlfq_level1 = (runqueue *)malloc(sizeof(runqueue));
		mlfq_level2 = (runqueue *)malloc(sizeof(runqueue));
		mlfq_level3 = (runqueue *)malloc(sizeof(runqueue));

		//initializing the queue for those with blocked threads
		blockedThreads = (runqueue *)malloc(sizeof(runqueue));

		//registering signal handler when a timer interrupt occurs
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = &timerHandler;
		sigaction(SIGPROF, &sa, NULL);

		//Assigning exit value NULL for each thread and terminated value 0
		for (int i =0; i<150; i++){
			threadsTerminated[i] = 0;
			thread_returnValues[i] = NULL;	
		
		}

		// Initializing the scheduler context
		if (getcontext(&schedulerContext) < 0){
			perror("getcontext");
			exit(1);
		}
		//Allocating the fields for scheduler context
		void *stack=malloc(STACK_SIZE);
		schedulerContext.uc_link=NULL;
		schedulerContext.uc_stack.ss_sp=stack;
		schedulerContext.uc_stack.ss_size=STACK_SIZE;
		schedulerContext.uc_stack.ss_flags=0;

		// modifying scheduler context to run the schedule function
		makecontext(&schedulerContext, schedule, 0);

	}

}

/* create a new thread */
int mypthread_create(mypthread_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {

       // YOUR CODE HERE	
	
	   // create a Thread Control Block
	   // create and initialize the context of this thread
	   // allocate heap space for this thread's stack
	   // after everything is all set, push this thread into the ready queue

       initialize();
	
     // Initializing main thread context and adding to shceduler queue  
	if (mainThrdstarted == 0){
		
		getcontext(&mainContext);
		if(mainThrdstarted == 0)
		{
				
			mainThread = (mypthread *)malloc(sizeof(mypthread));
			mainThread -> threadControlBlock = (tcb *)malloc(sizeof(tcb));
			mainThread -> threadControlBlock -> threadId = thrIdValue;
			thrIdValue ++;
	        mainThread -> threadControlBlock -> status = READY;	
			mainThread -> threadControlBlock -> ctx = mainContext;
			mainThread -> threadControlBlock -> priority = 4;
			addQueue(mainThread, runningQueue);
			//Assigning 1 to started so only one main thread is created
			mainThrdstarted = 1;

			//Transfer to scheduler context
			setcontext(&schedulerContext);

		}

	}

	//creating the thread  
	mypthread * newThread = (mypthread *)malloc(sizeof (mypthread));

	//Allocating memory for thread control block
	newThread -> threadControlBlock = (tcb *)malloc(sizeof(tcb));


	//setting up the context and stack for this thread
	ucontext_t nctx;
	if (getcontext(&nctx) < 0){
		perror("getcontext");
		exit(1);
	}
	void * thisStack = malloc(STACK_SIZE);
	nctx.uc_link = NULL;
	nctx.uc_stack.ss_sp = thisStack;
	nctx.uc_stack.ss_size = STACK_SIZE;
	nctx.uc_stack.ss_flags = 0;
	
	//Modifying the context of thread by passing the function and arguments
	if (arg == NULL){
		makecontext(&nctx, (void *)function, 0);
	}else{
		makecontext(&nctx, (void *)function, 1,arg);
	}
	
	//assigning a unique id to this thread
	*thread = thrIdValue;
	(newThread -> threadControlBlock) -> threadId = *thread;
	thrIdValue++;

	//updating the status to READY state
	(newThread -> threadControlBlock) -> status = READY;

	//Updating the priority of thread
	(newThread -> threadControlBlock) -> priority = 4;

	//Update the tcb context for this thread
	(newThread -> threadControlBlock) -> ctx = nctx;
	
	//Add to job queue
	addQueue(newThread, runningQueue);
	
    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int mypthread_yield() {
	
	// - change thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE
	
	(currentThread -> threadControlBlock) -> status = READY;
	
	//This lets the scheduler for MLFQ know that the thread has yielded before its time slice
	threadYield = 1;

	//Disabling the timer for thread and then performing context switch to scheduler context
	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 0;

	swapcontext(&((currentThread -> threadControlBlock)->ctx), &schedulerContext);
		
	return 0;
};

/* terminate a thread */
void mypthread_exit(void *value_ptr) {
	// preserve the return value pointer if not NULL
	// - de-allocate any dynamic memory created when starting this thread

	// YOUR CODE HERE

	//Assigning the value_ptr to thread exit value for each thread

	int index = currentThread -> threadControlBlock ->threadId; 

	if(value_ptr != NULL){
		thread_returnValues[index] = value_ptr;
	}else{
		thread_returnValues[index] = NULL;
	}
	

	//indicating this thread has exited 
	threadsTerminated[index] = 1;
	
	freeCurrentThread(currentThread);

	//Making current thread to null
	currentThread = NULL;

	//Disabling the timer for thread and then performing context switch to scheduler context

	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 0;

	gettimeofday(&tv, NULL);
	unsigned long time = 1000000 * tv.tv_sec + tv.tv_usec;

	threadCompletionTime[index] = time;
	
	//long long int turnaroundTime_thread = threadCompletionTime[index] - threadArrivalTime[index];
	//long long int responseTime_thread = threadScheduleTime[index] - threadArrivalTime[index];

	setcontext(&schedulerContext);

};

void freeCurrentThread(mypthread * thread){

	//Deallocating all the dynamic memory created for this thread
	free(((thread -> threadControlBlock) -> ctx).uc_stack.ss_sp);
	free(thread -> threadControlBlock);
	free(thread);

}

/* Wait for thread termination */
int mypthread_join(mypthread_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE

	// The exit array will be 1 if thread has exited, we need to wait
	// until the thread exited to perform a thread_join.
	// the threadTerminated value for the thread will get 1 so waiting until
	// the thread gets exited
	while (threadsTerminated[thread]==0){

	}

	//Store the value returned by the exited array in value_ptr

	if (value_ptr != NULL){
		
		*value_ptr = thread_returnValues[thread];

	}

	return 0;
};

/* initialize the mutex lock */
int mypthread_mutex_init(mypthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	// YOUR CODE HERE
	
	//initialize data structures for this mutex

	//invalid pointer check
	if(mutex == NULL){
		return -1;
	}

	//flag initialized to 0
	mutex->flag = 0;

	return 0;
};

/* aquire a mutex lock */
int mypthread_mutex_lock(mypthread_mutex_t *mutex)
{
		// YOUR CODE HERE
	
		// use the built-in test-and-set atomic function to test the mutex
		// if the mutex is acquired successfully, return
		// if acquiring mutex fails, put the current thread on the blocked/waiting list and context switch to the scheduler thread

		//using the test-and-set function to text mutex 
		if(!(__atomic_test_and_set (&mutex->flag, 1) == 0)){

			//mutex acquiring failed, current thread put on blocked list
			currentThread -> threadControlBlock -> status = BLOCKED;
			addQueue(currentThread, blockedThreads);

			//to make sure the thread isnt scheduled, 1 is assigned to isthreadblocked
			isThreadBlocked = 1;

			//context switching to scheduler thread
			swapcontext(&((currentThread->threadControlBlock)->ctx), &schedulerContext);

		}
		
		//mutex's thread is assigned to current thread as mutex is acquired successfully
		mutex-> mthread = currentThread;
		mutex->flag = 1;
		return 0;
};

/* release the mutex lock */
int mypthread_mutex_unlock(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE	
	
	// update the mutex's metadata to indicate it is unlocked
	// put the thread at the front of this mutex's blocked/waiting queue in to the run queue

	mutex -> flag = 0;
	mutex -> mthread = NULL;
	//releasing the threads in the blocked list into the run queue
	releaseThreads();
	return 0;
};


/* destroy the mutex */
int mypthread_mutex_destroy(mypthread_mutex_t *mutex)
{
	// YOUR CODE HERE
	
	// deallocate dynamic memory allocated during mypthread_mutex_init
	if(mutex==NULL){
		return -1;
	}
	else{
		return 0;
	}

	return 0;
};

/* scheduler */
static void schedule() {

	// YOUR CODE HERE
	
	// each time a timer signal occurs your library should switch in to this context
	
	// be sure to check the SCHED definition to determine which scheduling algorithm you should run
	//   i.e. RR, PSJF or MLFQ

// - scheduling policy
#ifndef RR
		// Choose MLFQ
     		// CODE 1
     		sched_MLFQ();
	#else 
		// Choose RR
     		// CODE 2
     		sched_RR(runningQueue);
	#endif

}

/* Round-robin scheduling algorithm */
static void sched_RR(runqueue * queue) {

	// YOUR CODE HERE

	// - your own implementation of RR
	// (feel free to modify arguments and return types)

	#ifndef MLFQ

		if (currentThread != NULL && isThreadBlocked != 1){
			currentThread -> threadControlBlock -> status = READY;
			addQueue(currentThread, runningQueue);
		}
	#endif
	// if atleast one job is present
	if (queue->head !=NULL){

		//popping head from scheduler which will run and assigning head to next job
		node * job = queue -> head;
		queue -> head = queue -> head -> next;

		//For only 1 job in queue
		if (queue -> head == NULL){
			queue -> tail = NULL;
		}

		//isolating the job and pointing its next ot NULL
		job->next = NULL;

		
		currentThread = job -> t;
		isThreadBlocked = 0;
		currentThread -> threadControlBlock -> status = SCHEDULED;

		free(job);

		//Assigning timer to the time slice of job
		timer.it_value.tv_usec = QUANTUMTIME*1000;
		timer.it_value.tv_sec = 0;
		setitimer(ITIMER_PROF, &timer, NULL);

		gettimeofday(&tv, NULL);
		unsigned long time = 1000000 * tv.tv_sec + tv.tv_usec;

		threadScheduleTime[currentThread->threadControlBlock->threadId] = time;
		
		setcontext(&(currentThread -> threadControlBlock->ctx));
		
		
	}

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_PSJF()
{
	// YOUR CODE HERE

	// Your own implementation of PSJF (STCF)
	// (feel free to modify arguments and return types)

	return;
}

/* Preemptive MLFQ scheduling algorithm */
/* Graduate Students Only */
static void sched_MLFQ() {
	// YOUR CODE HERE

	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)
	
	// checking whether thread isnt blocked or hasnt exited
	if(currentThread != NULL && isThreadBlocked != 1){


		//getting priority of current thread
		int threadPriority = currentThread -> threadControlBlock -> priority;
		//updating status of current thread to READY
		currentThread -> threadControlBlock -> status = READY;

		if (threadYield == 1){
			
			//if the value is 1, thread yielded. We assign the job in same priority level queue.
			
			if(threadPriority == 1){
				addQueue(currentThread, mlfq_level1);
			}else if (threadPriority == 2){
				addQueue(currentThread, mlfq_level2);
			}else if (threadPriority == 3){
				addQueue(currentThread, mlfq_level3);
			}else{
				addQueue(currentThread, runningQueue);
			}
			
			//setting threadYield to 0 for next thread 
			threadYield = 0;

		}else{

			// if not yielded, thread is put in lower priority level queue
			if(threadPriority == 1){
				addQueue(currentThread, mlfq_level1);
			}else if (threadPriority== 2){
				currentThread -> threadControlBlock -> priority =1; 
				addQueue(currentThread, mlfq_level1);

			}else if (threadPriority == 3){
				
				currentThread -> threadControlBlock -> priority =2; 
				addQueue(currentThread, mlfq_level2);
			}else{
				
				currentThread -> threadControlBlock -> priority =3; 
				addQueue(currentThread, mlfq_level3);
			}
			
		}
	}

	//round robin is performed for jobs inside the queue from highest to lowest (i.e. level 4 to 1)
	if (runningQueue->head != NULL){
		sched_RR(runningQueue);
	}else if (mlfq_level3->head != NULL){
		sched_RR(mlfq_level3);
	}else if (mlfq_level2->head != NULL){
		sched_RR(mlfq_level2);
	}else if (mlfq_level1->head != NULL){
		sched_RR(mlfq_level1);
	}

}



