/*
 * Threads.h
 *
 *  Created on: Feb 2, 2020
 *      Author: johnhodson
 */

#ifndef THREADS_H_
#define THREADS_H_

#define JOYSTICK_FIFO 0
#define TEMP_FIFO 1
#define LIGHT_FIFO 2

semaphore_t sensor_mutex;
semaphore_t led_mutex;

void thread0(void);
void thread1(void);
void thread2(void);
void thread3(void);
void thread4(void);
void thread5(void);
void pthread0(void);
void pthread1(void);

#endif /* THREADS_H_ */
