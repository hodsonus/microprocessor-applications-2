/*
 * G8RTOS_Scheduler.h
 */

#ifndef G8RTOS_SCHEDULER_H_
#define G8RTOS_SCHEDULER_H_

#include <G8RTOS/G8RTOS_Structures.h>

/*********************************************** Sizes and Limits *********************************************************************/
#define MAX_THREADS 6
#define STACKSIZE 1024
#define OSINT_PRIORITY 7
/*********************************************** Sizes and Limits *********************************************************************/


/*********************************************** Enums ********************************************************************************/
typedef enum SchedulerRequestCode
{
    NO_ERR = 0,
    ERR_MAX_THREADS_SCHEDULED = -1,
    ERR_LAUNCHED_NO_THREADS = -2,
    ERR_UNKN_FAILURE = -3,
} SchedulerRequestCode;
/*********************************************** Enums ********************************************************************************/


/*********************************************** Public Variables *********************************************************************/

/* Holds the current time for the whole System */
extern uint32_t SystemTime;

/*********************************************** Public Variables *********************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes variables and hardware for G8RTOS usage
 */
void G8RTOS_Init();

/*
 * Starts G8RTOS Scheduler
 * 	- Initializes Systick Timer
 * 	- Sets Context to first thread
 * Returns: Error Code for starting scheduler. This will only return if the scheduler fails
 */
SchedulerRequestCode G8RTOS_Launch();

/*
 * Adds threads to G8RTOS Scheduler
 * 	- Checks if there are still available threads to insert to scheduler
 * 	- Initializes the thread control block for the provided thread
 * 	- Initializes the stack for the provided thread
 * 	- Sets up the next and previous tcb pointers in a round robin fashion
 * Param "threadToAdd": Void-Void Function to add as preemptable main thread
 * Returns: Error code for adding threads
 */
SchedulerRequestCode G8RTOS_AddThread(void (*threadToAdd)(void));

/*
 * Cooperatively yields CPU
 */
void G8RTOS_Yield();

/*********************************************** Public Functions *********************************************************************/

#endif /* G8RTOS_SCHEDULER_H_ */
