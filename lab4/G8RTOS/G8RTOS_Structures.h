/*
 * G8RTOS_Structure.h
 *
 *  Created on: Jan 12, 2017
 *      Author: Raz Aloni
 */

#ifndef G8RTOS_STRUCTURES_H_
#define G8RTOS_STRUCTURES_H_

#include <G8RTOS/G8RTOS_Semaphores.h>
#include <stdbool.h>

#define MAX_NAME_LENGTH 16
typedef uint32_t threadId_t;

/*********************************************** Data Structure Definitions ***********************************************************/

/*
 *  Thread Control Block:
 *      - Every thread has a Thread Control Block
 *      - The Thread Control Block holds information about the Thread Such as the Stack Pointer, Priority Level, and Blocked Status
 */

typedef struct tcb_t
{
    int32_t* sp;
    struct tcb_t* prev;
    struct tcb_t* next;
    bool alive;
    uint8_t priority;
    bool asleep;
    uint32_t sleep_cnt;
    semaphore_t* blocked;
    threadId_t threadID;
    char threadName[MAX_NAME_LENGTH];
} tcb_t;

/*
 *  Periodic Thread Control Block:
 *      - Holds a function pointer that points to the periodic thread to be executed
 *      - Has a period in us
 *      - Holds Current time
 *      - Contains pointer to the next periodic event - linked list
 */

typedef struct ptcb_t
{
    void (*handler)(void);
    uint32_t period;
    uint32_t exec_time;
    struct ptcb_t* prev;
    struct ptcb_t* next;
} ptcb_t;

/*********************************************** Data Structure Definitions ***********************************************************/


/*********************************************** Public Variables *********************************************************************/

tcb_t* CurrentlyRunningThread;

/*********************************************** Public Variables *********************************************************************/

#endif /* G8RTOS_STRUCTURES_H_ */
