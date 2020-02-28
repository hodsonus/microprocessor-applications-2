/*
 * G8RTOS_Semaphores.c
 */

/*********************************************** Dependencies and Externs *************************************************************/

#include <G8RTOS/G8RTOS_CriticalSection.h>
#include <G8RTOS/G8RTOS_Semaphores.h>
#include <stdint.h>
#include "msp.h"

/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes a semaphore to a given value
 * Param "s": Pointer to semaphore
 * Param "value": Value to initialize semaphore to
 * THIS IS A CRITICAL SECTION
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
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_WaitSemaphore(semaphore_t* s)
{
	// TODO - make nonblocking

    int32_t IBit_State = StartCriticalSection();

    while((*s) <= 0)
    {
        EndCriticalSection(IBit_State);
        IBit_State = StartCriticalSection();
    }

    (*s)--;

    EndCriticalSection(IBit_State);
}

/*
 * Signals the completion of the usage of a semaphore
 *  - Increments the semaphore value by 1
 *  - Unblocks any threads waiting on that semaphore
 * Param "s": Pointer to semaphore to be signaled
 * THIS IS A CRITICAL SECTION
 */
void G8RTOS_SignalSemaphore(semaphore_t* s)
{
	// TODO - implement unblocking

    int32_t IBit_State = StartCriticalSection();

    (*s)++;

    EndCriticalSection(IBit_State);
}

/*********************************************** Public Functions *********************************************************************/