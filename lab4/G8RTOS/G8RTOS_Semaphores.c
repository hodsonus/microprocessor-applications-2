/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include "G8RTOS_Scheduler.h"
#include "G8RTOS_CriticalSection.h"
#include "G8RTOS_Semaphores.h"
#include "msp.h"

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 */
void G8RTOS_InitSemaphore(semaphore_t* s, int32_t value)
{
    int32_t IBit_State = StartCriticalSection();

    (*s) = value;

    EndCriticalSection(IBit_State);
}

/*
 * No longer waits for semaphore
 *  - Decrements semaphore
 *  - Blocks thread is sempahore is unavalible
 * Param "s": Pointer to semaphore to wait on
 */
void G8RTOS_WaitSemaphore(semaphore_t* s)
{
    int32_t IBit_State = StartCriticalSection();

    (*s)--;

    // if the resource was not available
    if ( (*s) < 0 )
    {
        // block the currently running thread
        CurrentlyRunningThread->blocked = s;

        EndCriticalSection(IBit_State);

        // and yield the CPU
        G8RTOS_Yield();
    }
    else
    {
        // the resource was available and we can continue without blocking
        EndCriticalSection(IBit_State);
    }
}

/*
 * Signals the completion of the usage of a semaphore
 *  - Increments the semaphore value by 1
 *  - Unblocks any threads waiting on that semaphore
 * Param "s": Pointer to semaphore to be signaled
 */
void G8RTOS_SignalSemaphore(semaphore_t* s)
{
    int32_t IBit_State = StartCriticalSection();

    (*s)++;

    // if the resource was unavailable before we signaled
    if ( (*s) <= 0 )
    {
        // search for the first thread blocked on this semaphore
        tcb_t* thread = CurrentlyRunningThread->next;
        while (thread->blocked != s) thread = thread->next;

        // and unblock it
        thread->blocked = 0;
    }

    EndCriticalSection(IBit_State);
}

/*********************************************** Public Functions *********************************************************************/
