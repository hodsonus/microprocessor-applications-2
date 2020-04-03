/*
 * G8RTOS_Scheduler.c
 */

#include <stdint.h>
#include <string.h>
#include "msp.h"
#include "BSP.h"
#include "G8RTOS_Scheduler.h"
#include "G8RTOS_CriticalSection.h"


/*********************************************** Dependencies and Externs *************************************************************/

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
static int32_t threadStacks[MAX_THREADS][STACK_SIZE];

/* Periodic Event Threads
 * - An array of periodic events to hold pertinent information for each thread
 */
static ptcb_t periodicThreadControlBlocks[MAX_PTHREADS];

/*********************************************** Data Structures Used *****************************************************************/


/*********************************************** Private Variables ********************************************************************/

/*
 * Current Number of Threads currently in the scheduler
 */
static uint32_t NumberOfThreads;

/*
 * Current Number of Periodic Threads currently in the scheduler
 */
static uint32_t NumberOfPThreads;

/*
 * Counter used to generate unique thread IDs
 */
static uint16_t IDCounter;

/*********************************************** Private Variables ********************************************************************/


/*********************************************** Private Functions ********************************************************************/

/*
 * Initializes the Systick and Systick Interrupt
 * The Systick interrupt will be responsible for starting a context switch between threads
 */
static void InitSysTick()
{
    // initialize SysTick for a 1 ms system time tick
    SysTick->LOAD = ClockSys_GetSysFreq() / 1000;
    SysTick->VAL = 0;
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk |
                     SysTick_CTRL_TICKINT_Msk |
                     SysTick_CTRL_ENABLE_Msk;
}

/*
 * Chooses the next thread to run.
 */
void G8RTOS_Scheduler()
{
    /* Set tempNextThread to be the next thread in the linked list (allows for
     * round-robin scheduling of equal priorities) and iterate through all the
     * other threads */
    tcb_t* tempNextThread = CurrentlyRunningThread->next;
    int currentMaxPriority = 256;
    for (int i = 0; i < NumberOfThreads; ++i, tempNextThread = tempNextThread->next)
    {
        /* If tempNextThread is neither sleeping or blocked, we check if its
         * priority value is less than a currentMaxPriority value (initial
         * currentMaxPriority value will be 256) */
        if (tempNextThread->alive && !tempNextThread->asleep && tempNextThread->blocked == NULL && tempNextThread->priority < currentMaxPriority)
        {
            /* If it is, we set the CurrentlyRunningThread equal to the thread with the
             *  higher priority, and reinitialize the currentMaxPriority */
            CurrentlyRunningThread = tempNextThread;
            currentMaxPriority = tempNextThread->priority;
        }
    }
}

/*
 * SysTick Handler
 * The Systick Handler now will increment the system time,
 * set the PendSV flag to start the scheduler,
 * and be responsible for handling sleeping and periodic threads
 */
void SysTick_Handler()
{
    // increment the system time
    ++SystemTime;

    // handle periodic threads if they exist
    if (NumberOfPThreads > 0)
    {
        // start the pointer at the first periodic thread
        ptcb_t* pthread = &periodicThreadControlBlocks[0];
        // and iterate over all of the pthreads
        for (int i = 0; i < NumberOfPThreads; ++i, pthread = pthread->next)
        {
            // if it is time for the ith pthread to execute
            if (pthread->exec_time == SystemTime)
            {
                // update the exec_time to the next time it should execute
                pthread->exec_time = SystemTime + pthread->period;

                // and execute the periodic task
                (pthread->handler)();
            }
        }
    }

    // wake up our sleeping threads if necessary
    // start the pointer at the current thread
    tcb_t* thread = CurrentlyRunningThread;
    // and iterate over all of the threads
    for (int i = 0; i < NumberOfThreads; ++i, thread = thread->next)
    {
        // if the current thread is asleep and it is time to wake it up
        if (thread->asleep && thread->sleep_cnt == SystemTime) {

            // wake it up
            thread->asleep = false;
        }
    }

    // yield the CPU preemptively
    G8RTOS_Yield();
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
void G8RTOS_Init(bool LCD_usingTP, playerType wifi_hostOrClient)
{
    // Initialize system time, number of threads, and ID counter to zero
    SystemTime = 0;
    NumberOfThreads = 0;
    NumberOfPThreads = 0;
    IDCounter = 0;

    // Relocate the VTOR table to SRAM
    uint32_t newVTORTable = 0x20000000;
    // 57 interrupt vectors to copy
    memcpy((uint32_t *)newVTORTable, (uint32_t *)SCB->VTOR, 57*4);
    SCB->VTOR = newVTORTable;

    // Initialize all hardware on the board
    BSP_InitBoard(LCD_usingTP, wifi_hostOrClient);
}

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes the SysTick
 * 	- Sets the priority of the SysTick and the PendSV interrupts
 * 	- Sets context to first thread to run (the one with the highest priority)
 * 	- Calls G8RTOS Start to initiate the first context switch and begin exec.
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
G8RTOS_Scheduler_Error G8RTOS_Launch()
{
    if (NumberOfThreads == 0) return NO_THREADS_SCHEDULED;

    // Set CurrentlyRunningThread to be the thread with the highest priority
    int min = -1;
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        if (threadControlBlocks[i].alive && (min == -1 || threadControlBlocks[i].priority < threadControlBlocks[min].priority))
        {
            min = i;
        }
    }
    if (min == -1) return UNKNOWN_FAILURE;
    CurrentlyRunningThread = &threadControlBlocks[min];

    // Initialize SysTick
    InitSysTick();

    // Set the priority of our OS (traditionally the lowest possible)
    __NVIC_SetPriority(PendSV_IRQn, PENDSV_PRIORITY);
    __NVIC_SetPriority(SysTick_IRQn, SYSTICK_PRIORITY);

    // Call G8RTOS_Start
    G8RTOS_Start();

    return UNKNOWN_FAILURE;
}

/*
 * Adds threads to G8RTOS Scheduler
 *  - Checks if there are still available threads to insert to scheduler
 *  - Initializes the thread control block for the provided thread
 *  - Initializes the stack for the provided thread
 *  - Sets up the next and previous tcb pointers in a round robin fashion
 * Param threadToAdd: Void-Void Function to add as preemptable main thread
 * Param priority: Priority of the thread that is being added. 0 is the
 *                   highest and 255 is the lowest priority.
 * Param thread_name: the name of the thread, helpful when debugging.
 * Returns: Error code for adding threads
 */
G8RTOS_Scheduler_Error G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char* thread_name)
{
    int32_t IBit_State = StartCriticalSection();

    // Checks if there are still available threads to insert to scheduler
    if (NumberOfThreads >= MAX_THREADS)
    {
        EndCriticalSection(IBit_State);
        return THREAD_LIMIT_REACHED;
    }

    // tcbToInitialize will be the first TCB not alive
    int tcbToInitialize = -1;
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        if (!threadControlBlocks[i].alive)
        {
            tcbToInitialize = i;
            break;
        }
    }

    // If there are no threads that are dead
    if (tcbToInitialize == -1)
    {
        EndCriticalSection(IBit_State);
        return THREADS_INCORRECTLY_ALIVE;
    }

    // Sets up the next and previous pointers
    if (NumberOfThreads == 0)
    {
        // If this is the first thread, point it to itself
        threadControlBlocks[tcbToInitialize].prev = &threadControlBlocks[tcbToInitialize];
        threadControlBlocks[tcbToInitialize].next = &threadControlBlocks[tcbToInitialize];
    }
    else
    {
        /* The old logic arranged pointers in the exact order they were
         * allocated in the array. This is no longer possible with dynamic
         * thread deletion and allocation, so we now insert the new thread
         * immediately after an arbitrary thread that is alive. */

        for (int i = 0; i < MAX_THREADS; ++i)
        {
            if (threadControlBlocks[i].alive)
            {
                // Arrange the new TCB's pointers to look as though it came after the alive thread
                threadControlBlocks[tcbToInitialize].prev = &threadControlBlocks[i];
                threadControlBlocks[tcbToInitialize].next = threadControlBlocks[i].next;

                // Arrange pointers in the other alive threads to point to the new TCB
                threadControlBlocks[i].next = &threadControlBlocks[tcbToInitialize];
                threadControlBlocks[tcbToInitialize].next->prev = &threadControlBlocks[tcbToInitialize];

                break;
            }
        }
    }

    // Sets stack tcb stack pointer to top of thread stack
    threadControlBlocks[tcbToInitialize].sp = &threadStacks[tcbToInitialize][STACK_SIZE-16];

    // Initializes the stack for the provided thread to hold a "fake context"
    threadStacks[tcbToInitialize][STACK_SIZE-1]  = THUMBBIT; // PSR
    threadStacks[tcbToInitialize][STACK_SIZE-2]  = (int32_t)threadToAdd; // R15 (PC)
    threadStacks[tcbToInitialize][STACK_SIZE-3]  = ZERO; // R14 (LR)
    threadStacks[tcbToInitialize][STACK_SIZE-4]  = ZERO; // R12
    threadStacks[tcbToInitialize][STACK_SIZE-5]  = ZERO; // R3
    threadStacks[tcbToInitialize][STACK_SIZE-6]  = ZERO; // R2
    threadStacks[tcbToInitialize][STACK_SIZE-7]  = ZERO; // R1
    threadStacks[tcbToInitialize][STACK_SIZE-8]  = ZERO; // R0
    threadStacks[tcbToInitialize][STACK_SIZE-9]  = ZERO; // R11
    threadStacks[tcbToInitialize][STACK_SIZE-10] = ZERO; // R10
    threadStacks[tcbToInitialize][STACK_SIZE-11] = ZERO; // R9
    threadStacks[tcbToInitialize][STACK_SIZE-12] = ZERO; // R8
    threadStacks[tcbToInitialize][STACK_SIZE-13] = ZERO; // R7
    threadStacks[tcbToInitialize][STACK_SIZE-14] = ZERO; // R6
    threadStacks[tcbToInitialize][STACK_SIZE-15] = ZERO; // R5
    threadStacks[tcbToInitialize][STACK_SIZE-16] = ZERO; // R4

    threadControlBlocks[tcbToInitialize].priority = priority;
    threadControlBlocks[tcbToInitialize].alive = true;
    threadControlBlocks[tcbToInitialize].asleep = false;
    threadControlBlocks[tcbToInitialize].blocked = NULL;
    threadControlBlocks[tcbToInitialize].thread_id = ((IDCounter++) << 16) | tcbToInitialize;
    strcpy(threadControlBlocks[tcbToInitialize].thread_name, thread_name);

    ++NumberOfThreads;

    EndCriticalSection(IBit_State);
    return SCHEDULER_NO_ERROR;
}

/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param PthreadToAdd: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
G8RTOS_Scheduler_Error G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period)
{
    int32_t IBit_State = StartCriticalSection();

    // Checks if there are still available threads to insert to scheduler
    if (NumberOfPThreads >= MAX_PTHREADS)
    {
        EndCriticalSection(IBit_State);
        return PTHREAD_LIMIT_REACHED;
    }

    if (NumberOfPThreads == 0)
    {
        // If this is the first thread, point it to itself
        periodicThreadControlBlocks[NumberOfPThreads].prev = &periodicThreadControlBlocks[NumberOfPThreads];
        periodicThreadControlBlocks[NumberOfPThreads].next = &periodicThreadControlBlocks[NumberOfPThreads];
    }
    else
    {
        // Insert the new thread immediately after the most recently inserted thread
        periodicThreadControlBlocks[NumberOfPThreads].prev = &periodicThreadControlBlocks[NumberOfPThreads-1];
        periodicThreadControlBlocks[NumberOfPThreads-1].next = &periodicThreadControlBlocks[NumberOfPThreads];

        // Point the ends towards each other
        periodicThreadControlBlocks[NumberOfPThreads].next = &periodicThreadControlBlocks[0];
        periodicThreadControlBlocks[0].prev = &periodicThreadControlBlocks[NumberOfPThreads];
    }

    periodicThreadControlBlocks[NumberOfPThreads].handler = PthreadToAdd;
    periodicThreadControlBlocks[NumberOfPThreads].exec_time = SystemTime + period;
    periodicThreadControlBlocks[NumberOfPThreads].period = period;

    ++NumberOfPThreads;

    EndCriticalSection(IBit_State);
    return SCHEDULER_NO_ERROR;
}

/*
 * Puts the current thread into a sleep state.
 * param durationMS: Duration of sleep time in ms
 */
void G8RTOS_Sleep(uint32_t duration)
{
    // sleep_cnt is the time that this thread should be woken up
    CurrentlyRunningThread->sleep_cnt = SystemTime + duration;

    // set the sleep flag so the scheduler knows to skip this thread
    CurrentlyRunningThread->asleep = true;

    // yield the CPU
    G8RTOS_Yield();
}

/*
 * Yields the rest of the CRT's time if used cooperatively. Can also be
 * used by the OS to force context switches when threads are killed.
 */
void G8RTOS_Yield()
{
    // set the PendSV flag to start the scheduler
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*
 * Returns the CRT's thread id.
 */
threadId_t G8RTOS_GetThreadId()
{
    return CurrentlyRunningThread->thread_id;
}

/*
 * Kill all threads, except for the CRT.
 */
void G8RTOS_KillAllOtherThreads()
{
    for (int i = 0; i < NumberOfThreads; ++i)
    {
        if (&threadControlBlocks[i] != CurrentlyRunningThread) G8RTOS_KillThread(threadControlBlocks[i].thread_id);
    }
}

/*
 * Kill the thread with id threadId.
 */
G8RTOS_Scheduler_Error G8RTOS_KillThread(threadId_t threadId)
{
    // Enter a critical section
    int32_t IBit_State = StartCriticalSection();

    // Return appropriate error code if there’s only one thread running
    if (NumberOfThreads == 1)
    {
        EndCriticalSection(IBit_State);
        return CANNOT_KILL_LAST_THREAD;
    }

    // Search for thread with the same threadId
    int thread_to_kill = -1;
    for (int i = 0; i < MAX_THREADS; ++i)
    {
        if (threadControlBlocks[i].alive && threadControlBlocks[i].thread_id == threadId)
        {
            thread_to_kill = i;
            break;
        }
    }

    // Return error code if the thread does not exist
    if (thread_to_kill == -1)
    {
        EndCriticalSection(IBit_State);
        return THREAD_DOES_NOT_EXIST;
    }

    // Set the threads isAlive bit to false
    threadControlBlocks[thread_to_kill].alive = false;

    // Update thread pointers
    threadControlBlocks[thread_to_kill].next->prev = threadControlBlocks[thread_to_kill].prev;
    threadControlBlocks[thread_to_kill].prev->next = threadControlBlocks[thread_to_kill].next;

    // If thread being killed is the currently running thread, we need to context switch
    if (threadControlBlocks[thread_to_kill].thread_id == CurrentlyRunningThread->thread_id)
    {
        G8RTOS_Yield();
    }

    // Decrement number of threads
    --NumberOfThreads;

    // End critical section
    EndCriticalSection(IBit_State);

    return SCHEDULER_NO_ERROR;
}

/*
 * Kill the CRT.
 */
G8RTOS_Scheduler_Error G8RTOS_KillSelf()
{
    return G8RTOS_KillThread(CurrentlyRunningThread->thread_id);
}

/*
 * Add an aperiodic event thread (essentially an interrupt routine) by
 * initializing appropriate NVIC registers.
 */
G8RTOS_Scheduler_Error G8RTOS_AddAperiodicEvent(void (*AthreadToAdd)(void), uint8_t priority, IRQn_Type IRQn)
{
    int32_t IBit_State = StartCriticalSection();

    /* Verify the IRQn is not less than the last exception (PSS_IRQn) or
     * greater than the last acceptable user IRQn (PORT6_IRQn). */
    if (IRQn < PSS_IRQn || IRQn > PORT6_IRQn)
    {
        EndCriticalSection(IBit_State);
        return IRQn_INVALID;
    }

    /* Verify priority is not greater than 6 (the greatest user priority
     * number). */
    if (priority > 6)
    {
        EndCriticalSection(IBit_State);
        return HWI_PRIORITY_INVALID;
    }

    // Initialize the NVIC registers
    __NVIC_SetVector(IRQn, (uint32_t)AthreadToAdd);
    __NVIC_SetPriority(IRQn, (uint32_t)priority);
    NVIC_EnableIRQ(IRQn);

    EndCriticalSection(IBit_State);
    return SCHEDULER_NO_ERROR;
}

/*********************************************** Public Functions *********************************************************************/
