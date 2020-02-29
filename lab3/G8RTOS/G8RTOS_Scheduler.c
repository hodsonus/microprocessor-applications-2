/*
 * G8RTOS_Scheduler.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <G8RTOS/G8RTOS_Scheduler.h>
#include <stdint.h>
#include "msp.h"
#include "BSP.h"

/*
 * G8RTOS_Start exists in asm
 */
extern void G8RTOS_Start();

/* System Core Clock From system_msp432p401r.c */
extern uint32_t SystemCoreClock;

/*
 * Pointer to the currently running Thread Control Block
 */
extern tcb_t * CurrentlyRunningThread;

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines ******************************************************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT 0x01000000
/* Default Register Values */
#define ZERO 0x0000

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

/* Thread Control Blocks
 *	- An array of thread control blocks to hold pertinent information for each thread
 */
static tcb_t threadControlBlocks[MAX_THREADS];

/* Thread Stacks
 *	- An array of arrays that will act as individual stacks for each thread
 */
static int32_t threadStacks[MAX_THREADS][STACKSIZE];

/* Periodic Event Threads
 * - An array of periodic events to hold pertinent information for each thread
 */
static ptcb_t Pthread[MAX_PTHREADS];

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPthreads;

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 * Param "numCycles": Number of cycles for each systick interrupt - use as ClockSys_GetSysFreq() / 10^3 to overflow every 1ms
 */
static void InitSysTick(uint32_t numCycles)
{
    SysTick_Config(numCycles);
}

/*
 * Chooses the next thread to run.
 * Lab 2 Scheduling Algorithm:
 * 	- Simple Round Robin: Choose the next running thread by selecting the currently running thread's next pointer
 * 	- Check for sleeping and blocked threads
 */
void G8RTOS_Scheduler()
{
    // TODO - Check for sleeping threads

    tcb_t* thread_to_schedule = CurrentlyRunningThread->next;
    while (thread_to_schedule->blocked != 0) thread_to_schedule = thread_to_schedule->next;

    CurrentlyRunningThread = thread_to_schedule;
}

/*
 * SysTick Handler
 * The Systick Handler now will increment the system time,
 * set the PendSV flag to start the scheduler,
 * and be responsible for handling sleeping and periodic threads
 */
void SysTick_Handler()
{
    // TODO - handle sleeping and periodic threads

    // increment the system time
    SystemTime++;

    // set the PendSV flag to start the scheduler
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*********************************************** Private Functions ********************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Sets variables to an initial state (system time and number of threads)
 * Enables board for highest speed clock and disables watchdog
 */
void G8RTOS_Init()
{
    // Initialize system time to zero
    SystemTime = 0;

    // Set the number of threads to zero
    NumberOfThreads = 0;

    // Initialize all hardware on the board
    BSP_InitBoard();
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the Systick
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
SchedulerRequestCode G8RTOS_Launch()
{
    if (NumberOfThreads == 0) return ERR_LAUNCHED_NO_THREADS;

    // Set CurrentlyRunningThread
    CurrentlyRunningThread = &threadControlBlocks[0];

    // Initialize SysTick
    InitSysTick(ClockSys_GetSysFreq() / 10^3);

    // Set the priorities of PendSV and SysTick to the lowest priority
    __NVIC_SetPriority(PendSV_IRQn, OSINT_PRIORITY);
    __NVIC_SetPriority(SysTick_IRQn, OSINT_PRIORITY);

    // Call G8RTOS_Start
    G8RTOS_Start();

    return ERR_UNKN_FAILURE;
}


/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are still available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread to hold a "fake context"
 * 	- Sets stack tcb stack pointer to top of thread stack
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
SchedulerRequestCode G8RTOS_AddThread(void (*threadToAdd)(void))
{
    // Checks if there are still available threads to insert to scheduler
    if (NumberOfThreads >= MAX_THREADS) return ERR_MAX_THREADS_SCHEDULED;

    // Initializes the thread control block for the provided thread
    // Sets up the next and previous tcb pointers in a round robin fashion
    // The pointers below are arranged the same as threadControlBlocks array
    // NumberOfThreads points to the new thread location
    if (NumberOfThreads == 0)
    {
        // If this is the first thread, point it to itself
        threadControlBlocks[NumberOfThreads].prev = &threadControlBlocks[NumberOfThreads];
        threadControlBlocks[NumberOfThreads].next = &threadControlBlocks[NumberOfThreads];
    }
    else
    {
        // Insert the new thread immediately after the most recently inserted thread
        threadControlBlocks[NumberOfThreads].prev = &threadControlBlocks[NumberOfThreads-1];
        threadControlBlocks[NumberOfThreads-1].next = &threadControlBlocks[NumberOfThreads];

        // Point the ends towards each other
        threadControlBlocks[NumberOfThreads].next = &threadControlBlocks[0];
        threadControlBlocks[0].prev = &threadControlBlocks[NumberOfThreads];
    }

    // Sets stack tcb stack pointer to top of thread stack
    threadControlBlocks[NumberOfThreads].sp = &threadStacks[NumberOfThreads][STACKSIZE-16];

    // Initializes the stack for the provided thread to hold a "fake context"
    threadStacks[NumberOfThreads][STACKSIZE-1]  = THUMBBIT; // PSR
    threadStacks[NumberOfThreads][STACKSIZE-2]  = (int32_t)threadToAdd; // R15 (PC)
    threadStacks[NumberOfThreads][STACKSIZE-3]  = ZERO; // R14 (LR)
    threadStacks[NumberOfThreads][STACKSIZE-4]  = ZERO; // R12
    threadStacks[NumberOfThreads][STACKSIZE-5]  = ZERO; // R3
    threadStacks[NumberOfThreads][STACKSIZE-6]  = ZERO; // R2
    threadStacks[NumberOfThreads][STACKSIZE-7]  = ZERO; // R1
    threadStacks[NumberOfThreads][STACKSIZE-8]  = ZERO; // R0
    threadStacks[NumberOfThreads][STACKSIZE-9]  = ZERO; // R11
    threadStacks[NumberOfThreads][STACKSIZE-10] = ZERO; // R10
    threadStacks[NumberOfThreads][STACKSIZE-11] = ZERO; // R9
    threadStacks[NumberOfThreads][STACKSIZE-12] = ZERO; // R8
    threadStacks[NumberOfThreads][STACKSIZE-13] = ZERO; // R7
    threadStacks[NumberOfThreads][STACKSIZE-14] = ZERO; // R6
    threadStacks[NumberOfThreads][STACKSIZE-15] = ZERO; // R5
    threadStacks[NumberOfThreads][STACKSIZE-16] = ZERO; // R4

    NumberOfThreads++;

    return NO_ERR;
}

/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param Pthread To Add: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
int G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period)
{
    /* TODO - Implement this */
    return -1;
}

/*
 * Puts the current thread into a sleep state.
 *  param durationMS: Duration of sleep time in ms
 */
void sleep(uint32_t durationMS)
{
    /* TODO - Implement this */
}

/*
 * Cooperatively yields CPU
 */
void G8RTOS_Yield()
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*********************************************** Public Functions *********************************************************************/
