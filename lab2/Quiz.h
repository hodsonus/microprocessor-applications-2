/*
 * Quiz.h
 *
 *  Created on: Feb 21, 2020
 *      Author: johnhodson
 */

#ifndef QUIZ_H_
#define QUIZ_H_

#include <stdint.h>
#include <G8RTOS/G8RTOS.h>

void Thread0(void);
void Thread1(void);
void Thread2(void);
void Thread3(void);
void ResetThread(void);
void LEDThread(void);

uint8_t counter0;
uint8_t counter1;
uint8_t counter2;
uint8_t counter3;

semaphore_t sem_counter0;
semaphore_t sem_counter1;
semaphore_t sem_counter2;
semaphore_t sem_counter3;
semaphore_t sem_leds;

#endif /* QUIZ_H_ */
