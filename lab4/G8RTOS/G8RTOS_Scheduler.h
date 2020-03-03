/*
 * G8RTOS_Scheduler.h
 */

#ifndef G8RTOS_SCHEDULER_H_
#define G8RTOS_SCHEDULER_H_

#include "G8RTOS_Structures.h"
#include "msp.h"

/*********************************************** Sizes and Limits *********************************************************************/
#define MAX_THREADS 6
#define MAX_PTHREADS 6
#define STACK_SIZE 1024
#define PENDSV_PRIORITY 7
#define SYSTICK_PRIORITY 3
/*********************************************** Sizes and Limits *********************************************************************/


/*********************************************** Enums ********************************************************************************/
typedef enum G8RTOS_Scheduler_Error
{
    SCHEDULER_NO_ERROR = 0,
    THREAD_LIMIT_REACHED = -1,
    NO_THREADS_SCHEDULED = -2,
    THREADS_INCORRECTLY_ALIVE = -3,
    THREAD_DOES_NOT_EXIST = -4,
    CANNOT_KILL_LAST_THREAD = -5,
    IRQn_INVALID = -6,
    HWI_PRIORITY_INVALID = -7,
    PTHREAD_LIMIT_REACHED = -8,
    UNKNOWN_FAILURE = -9,
} G8RTOS_Scheduler_Error;
/*********************************************** Enums ********************************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
extern uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes variables and hardware for G8RTOS usage
 */
void G8RTOS_Init(bool LCD_usingTP);

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes Systick Timer
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
G8RTOS_Scheduler_Error G8RTOS_Launch();

/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are still available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param threadToAdd: Void-Void Function to add as preemptable main thread
 * Param priority: Priority of the thread that is being added. 0 is the
 *                   highest and 255 is the lowest priority.
 * Param thread_name: the name of the thread, helpful when debugging.
 * Returns: Error code for adding threads
 */
G8RTOS_Scheduler_Error G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char* thread_name);

/*
 * Adds periodic threads to G8RTOS Scheduler
 * Function will initialize a periodic event struct to represent event.
 * The struct will be added to a linked list of periodic events
 * Param PthreadToAdd: void-void function for P thread handler
 * Param period: period of P thread to add
 * Returns: Error code for adding threads
 */
G8RTOS_Scheduler_Error G8RTOS_AddPeriodicEvent(void (*PthreadToAdd)(void), uint32_t period);

/*
 * Puts the current thread into a sleep state.
 * Param duration: Duration of sleep time in ms
 */
void G8RTOS_Sleep(uint32_t duration);

/*
 * Yields the rest of the CRT's time if used cooperatively. Can also be
 * used by the OS to force context switches when threads are killed.
 */
void G8RTOS_Yield();

/*
 * Returns the CRT's thread id.
 */
threadId_t G8RTOS_GetThreadId();

/*
 * Kill the thread with id threadId.
 */
G8RTOS_Scheduler_Error G8RTOS_KillThread(threadId_t threadId);

/*
 * Kill the CRT.
 */
G8RTOS_Scheduler_Error G8RTOS_KillSelf();

/*
 * Add an aperiodic event thread (essentially an interrupt routine) by
 * initializing appropriate NVIC registers.
 */
G8RTOS_Scheduler_Error G8RTOS_AddAPeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, IRQn_Type IRQn);

/*********************************************** Public Functions *********************************************************************/

#endif /* G8RTOS_SCHEDULER_H_ */
