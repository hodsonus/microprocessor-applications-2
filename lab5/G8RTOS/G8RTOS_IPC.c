/*
 * G8RTOS_IPC.c
 *
 *  Created on: Feb 29, 2020
 *      Author: John Hodson
 */

#include <stdint.h>
#include "msp.h"
#include "G8RTOS_IPC.h"
#include "G8RTOS_Semaphores.h"


/*********************************************** Defines ******************************************************************************/

#define FIFO_SIZE 16
#define MAX_NUMBER_OF_FIFOS 4

/*********************************************** Defines ******************************************************************************/


/*********************************************** Data Structures Used *****************************************************************/

typedef struct FIFO_t {
    int32_t buffer[FIFO_SIZE];
    int32_t* head;
    int32_t* tail;
    uint32_t lost_data;
    semaphore_t current_size;
    semaphore_t mutex;
} FIFO_t;

/* Array of FIFOS */
static FIFO_t FIFOs[4];

/*********************************************** Data Structures Used *****************************************************************/

/*
 * Initializes FIFO i
 * Param "i": which buffer we wish to initialize
 * Returns: error code (G8RTOS_FIFO_Error)
 */
G8RTOS_FIFO_Error G8RTOS_InitFIFO(uint32_t i)
{
    if (i >= MAX_NUMBER_OF_FIFOS) return ERR_FIFO_INDEX;

    FIFOs[i].head = &(FIFOs[i].buffer[0]);
    FIFOs[i].tail = &(FIFOs[i].buffer[0]);
    FIFOs[i].lost_data = 0;
    G8RTOS_InitSemaphore(&(FIFOs[i].current_size), 0);
    G8RTOS_InitSemaphore(&(FIFOs[i].mutex), 1);

    return OK_FIFO;
}

/*
 * Reads from FIFO i
 *  - Waits until current_size semaphore is greater than zero
 *  - Gets data and increments head (wrapping if necessary)
 * Param: "i": which buffer we want to read from
 * Returns: int32_t data from FIFO i
 */
int32_t G8RTOS_ReadFIFO(uint32_t i)
{
    /* TODO - how can we return errors?
     * Perhaps we pass in a pointer to an integer and fill it with data.
     * It is valid only if we receive OK as return value.
     * If this is the desired soln., edit the method signature */

    // if the order of the two below waits are changed, deadlocks can occur

    // claim an element inside the FIFO, blocking if it doesn't exist yet
    G8RTOS_WaitSemaphore(&(FIFOs[i].current_size));
    // wait for exclusive access to the FIFO
    G8RTOS_WaitSemaphore(&(FIFOs[i].mutex));

    // read the data from the FIFO
    int32_t data = *(FIFOs[i].head);

    /* increment the head and wrap the head if needed (if we are pointing to
     * an element outside of the buffer) */
    ++FIFOs[i].head;
    if (FIFOs[i].head >= &(FIFOs[i].buffer[FIFO_SIZE]))
    {
        FIFOs[i].head = &(FIFOs[i].buffer[0]);
    }

    // allow others exclusive access to the FIFO
    G8RTOS_SignalSemaphore(&(FIFOs[i].mutex));

    return data;
}

/*
 * Writes to FIFO i
 *  - Writes data to tail of the buffer and increments tail (wrapping if
 *    necessary)
 *  Param "i": which buffer we want to read from
 *        "data': data being put into FIFO
 *  Returns: error code (G8RTOS_FIFO_Error)
 */
G8RTOS_FIFO_Error G8RTOS_WriteFIFO(uint32_t i, int32_t data)
{
    if (i >= MAX_NUMBER_OF_FIFOS) return ERR_FIFO_INDEX;

    // default error status
    G8RTOS_FIFO_Error status = OK_FIFO;

    // wait for exclusive access to the FIFO
    G8RTOS_WaitSemaphore(&(FIFOs[i].mutex));

    // write the data at the tail (the next insertion point)
    *(FIFOs[i].tail) = data;

    // if we just overwrote data
    if (FIFOs[i].current_size > FIFO_SIZE - 1)
    {
        // increment our last data counter
        ++FIFOs[i].lost_data;

        /* advance the head to point at the oldest data (the previous oldest
         * data was just overwritten) and wrap if needed */
        ++FIFOs[i].head;
        if (FIFOs[i].head >= &(FIFOs[i].buffer[FIFO_SIZE]))
        {
            FIFOs[i].head = &(FIFOs[i].buffer[0]);
        }

        // change the status from the default to an error
        status = ERR_DATA_OVERWRITTEN;
    }
    else
    {
        // else we didn't overwrite any data, signal that our buffer has grown
        G8RTOS_SignalSemaphore(&(FIFOs[i].current_size));
    }

    // always advance the tail pointer and wrap if needed
    ++FIFOs[i].tail;
    if (FIFOs[i].tail >= &(FIFOs[i].buffer[FIFO_SIZE]))
    {
        FIFOs[i].tail = &(FIFOs[i].buffer[0]);
    }

    // signal and allow others exclusive access to the FIFO
    G8RTOS_SignalSemaphore(&(FIFOs[i].mutex));

    return status;
}

/*
 * Checks if FIFO i is empty
 *  - Can be used to prevent a blocking read.
 *  Param "i": which buffer we want to read from
 *  Returns: true if the buffer is empty
 */
bool G8RTOS_FIFOIsEmpty(uint32_t i)
{
    // wait for exclusive access to the FIFO
    G8RTOS_WaitSemaphore(&(FIFOs[i].mutex));

    bool is_empty = FIFOs[i].current_size == 0;

    // allow others exclusive access to the FIFO
    G8RTOS_SignalSemaphore(&(FIFOs[i].mutex));

    return is_empty;
}
