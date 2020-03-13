/*
 * Quiz.h
 *
 *  Created on: Mar 13, 2020
 *      Author: johnhodson
 */

#ifndef QUIZ_H_
#define QUIZ_H_

semaphore_t led_mutex;

void quizCoinAndCollisionThread(void);
void quizPlayerThread(void);
void quizEnemyThread(void);

void quizPThread(void);

inline void writeLeds(uint32_t color, uint16_t data);

#endif /* QUIZ_H_ */
