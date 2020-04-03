/*
 * G8RTOS_IPC.h
 *
 *  Created on: Feb 29, 2020
 *      Author: John Hodson
 */

#ifndef G8RTOS_IPC_H_
#define G8RTOS_IPC_H_

#include <stdbool.h>


/*********************************************** Error Codes **************************************************************************/
typedef enum G8RTOS_FIFO_Error
{
    OK_FIFO = 0,
    ERR_FIFO_INDEX = -1,
    ERR_DATA_OVERWRITTEN = -2,
} G8RTOS_FIFO_Error;
/*********************************************** Error Codes **************************************************************************/


/*********************************************** Public Functions *********************************************************************/

/*
 * Initializes One to One FIFO Struct
 */
G8RTOS_FIFO_Error G8RTOS_InitFIFO(uint32_t i);

/*
 * Reads FIFO
 *  - Waits until CurrentSize semaphore is greater than zero
 *  - Gets data and increments the head ptr (wraps if necessary)
 * Param "i": chooses which buffer we want to read from
 * Returns: int32_t Data from FIFO
 */
int32_t G8RTOS_ReadFIFO(uint32_t i);

/*
 * Writes to FIFO
 *  Writes data to tail of the buffer if the buffer is not full
 *  Increments tail (wraps if necessary)
 *  Param "i": chooses which buffer we want to read from
 *        "data': Data being put into FIFO
 *  Returns: error code for full buffer if unable to write
 */
G8RTOS_FIFO_Error G8RTOS_WriteFIFO(uint32_t i, int32_t data);

/*
 * Checks if FIFO i is empty
 *  - Can be used to prevent a blocking read.
 *  Param "i": which buffer we want to read from
 *  Returns: true if the buffer is empty
 */
bool G8RTOS_FIFOIsEmpty(uint32_t i);

/*********************************************** Public Functions *********************************************************************/


#endif /* G8RTOS_IPC_H_ */
