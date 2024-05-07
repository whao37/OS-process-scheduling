#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "os-sim.h"

typedef struct _list_node_t {
    struct _list_node_t* next;
    pcb_t* pcb;
} list_node_t;

static list_node_t* head = NULL;
static list_node_t* tail = NULL;

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex;
static pthread_cond_t rqueue_not_empty;

static void push_ready(pcb_t* pcb)
{
    list_node_t* node = malloc(sizeof(list_node_t));
    assert(node != NULL);
    node->next = NULL;
    node->pcb = pcb;
    
    if (head == NULL) head = node;
    else tail->next = node;
    tail = node;
}

static pcb_t* pop_ready()
{
    if (head == NULL) return NULL;
    list_node_t* node = head;
    pcb_t* pcb = node->pcb;
    head = head->next;
    free(node);
    return pcb;
}


/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running process indexed by the cpu id. 
 *	See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    pcb_t* pcb = pop_ready();
    if(pcb == NULL) context_switch(cpu_id, pcb, -1);
    else {
        pcb->state = PROCESS_RUNNING;
        context_switch(cpu_id, pcb, 15);
    }
    current[cpu_id] = pcb;
    pthread_mutex_unlock(&current_mutex);
}

/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    while (head == NULL) {
        pthread_cond_wait(&rqueue_not_empty, &current_mutex);
    }
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    pcb_t* pcb = current[cpu_id];
    pcb->state = PROCESS_READY;
    push_ready(pcb);
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    pthread_mutex_lock(&current_mutex);
    pcb_t* pcb = current[cpu_id];
    pcb->state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    current[cpu_id] = NULL;
    schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is LRTF, wake_up() may need
 *      to preempt the CPU with lower remaining time left to allow it to
 *      execute the process which just woke up with higher reimaing time.
 * 	However, if any CPU is currently running idle,
* 	or all of the CPUs are running processes
 *      with a higher remaining time left than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    pthread_mutex_lock(&current_mutex);
    process->state = PROCESS_READY;
    push_ready(process);
    pthread_cond_signal(&rqueue_not_empty);
    pthread_mutex_unlock(&current_mutex);
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -l and -r command-line parameters.
 */
int main(int argc, char *argv[])
{
    unsigned int cpu_count;

    if (argc != 2)
    {
        fprintf(stderr, "Multithreaded OS Simulator\n"
            "Usage: ./os-sim <# CPUs> [ -l | -r <time slice> ]\n"
            "    Default : FIFO Scheduler\n"
	    "         -l : Longest Remaining Time First Scheduler\n"
            "         -r : Round-Robin Scheduler\n\n");
        return -1;
    }
    cpu_count = strtoul(argv[1], NULL, 0);

    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_cond_init(&rqueue_not_empty, NULL);

    start_simulator(cpu_count);

    return 0;
}


