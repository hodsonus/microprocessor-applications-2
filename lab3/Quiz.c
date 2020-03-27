/*
 * Quiz.c
 *
 *  Created on: Mar 13, 2020
 *      Author: johnhodson
 */

#include <stdint.h>
#include <G8RTOS/G8RTOS.h>
#include <BSP.h>
#include "Quiz.h"

#define MAX_SPEED 100
#define DELTA_SPEED MAX_SPEED
#define ENEMY_INIT_SPEED MAX_SPEED * 8

#define ENEMY_INIT_POS BIT3
#define PLAYER_INIT_POS BITE

#define PLAYER_COLOR BLUE
#define COIN_COLOR GREEN
#define ENEMY_COLOR RED

#define MAX_COINS 4

#define THUMBSTICK_THRESHOLD 500

#define UPDATE_ENEMY_SPEED 5

#define PLAYER_SLEEP_COUNT 4
#define LENGTH_ENEMY_FLIP_COUNT 5
#define COIN_SLEEP_COUNT 5

typedef enum direction_t
{
    LEFT = -1,
    NONE = 0,
    RIGHT = 1,
} direction_t;

typedef struct player_t {
    uint16_t position;
    int speed;
    int points;
    bool awake;
    int sleep_count;
} player_t;

typedef struct enemy_t {
    uint16_t position;
    int speed;
    direction_t direction;
    int times_caught_player;
    int flip_count;
} enemy_t;

typedef struct coin_t {
    uint16_t position;
    int sleep_count;
    bool awake;
} coin_t;

static enemy_t enemy;
static player_t player;
static coin_t coins[MAX_COINS];

inline void WriteLeds(uint32_t color, uint16_t data) {
   G8RTOS_WaitSemaphore(&led_mutex);
    LP3943_LedModeSet(color, data);
   G8RTOS_SignalSemaphore(&led_mutex);
}

/* Responsible for the movement of the coins and detection of LED collisions */
void quizCoinAndCollisionThread(void) {
    // initialize the coin attributes
    coins[0].position = BIT0;
    coins[0].awake = true;
    coins[1].position = BIT4;
    coins[1].awake = true;
    coins[2].position = BIT8;
    coins[2].awake = true;
    coins[3].position = BITC;
    coins[3].awake = true;

    while (1) {

        uint16_t coin_positions = 0x0000;
        // detect coin and player collision, write coins to LEDs
        for (int i = 0; i < MAX_COINS; ++i) {
            // if there was a collision with an awake coin
            if (coins[i].awake && player.position & coins[i].position) {
                // increment player coins collected
                player.points += 1;

                if (player.points % UPDATE_ENEMY_SPEED == 0) {
                    enemy.speed -= DELTA_SPEED;
                    if (enemy.speed < MAX_SPEED) {
                        enemy.speed = MAX_SPEED;
                    }
                }

                // and delete myself (will be woken up by the PET)
                coins[i].awake = false;
                coins[i].sleep_count = COIN_SLEEP_COUNT;
            }
            else if (coins[i].awake) coin_positions |= coins[i].position;
        }
        WriteLeds(COIN_COLOR, coin_positions);

        // detect enemy and player collision
        if (player.awake && enemy.position & player.position) {

            // reset the player's position
            player.position = PLAYER_INIT_POS;

            // take a point away
            player.points -= 1;

            // put the player to sleep
            player.awake = false;
            player.sleep_count = PLAYER_SLEEP_COUNT;

            // and increment the amount of times that we have caught the player
            enemy.times_caught_player += 1;
        }

        G8RTOS_Yield();
    }

}

/* Responsible for the movement of the player on the LEDs */
void quizPlayerThread(void) {
    // initialize the player attributes
    player.position = PLAYER_INIT_POS;
    player.speed = MAX_SPEED;
    player.awake = true;

    // values used to zero the joystick on successive readings
    int16_t x_coord_zero, y_coord_zero;
    GetJoystickCoordinates(&x_coord_zero, &y_coord_zero);

    while (1) {

        int16_t x_coord, y_coord;

        if (player.awake) {
            // read data from joystick
            GetJoystickCoordinates(&x_coord, &y_coord);
            x_coord = x_coord - x_coord_zero;
            y_coord = y_coord - y_coord_zero;

            // check to see if the player is moving left or right and
            // update accordingly
            if (x_coord < -THUMBSTICK_THRESHOLD) {
                player.position = player.position << 1;
                if (player.position == 0) player.position = 0x0001;
            }
            else if (x_coord > THUMBSTICK_THRESHOLD) {
                player.position = player.position >> 1;
                if (player.position == 0) player.position = 0x8000;
            }
        }

        // update the player's position on the LEDs
        WriteLeds(PLAYER_COLOR, player.position);

        // sleep for speed amount of time
        G8RTOS_Sleep(player.speed);
    }
}

/* Responsible for the movement of the enemy. */
void quizEnemyThread(void) {
    // initialize the enemy attributes
    enemy.position = ENEMY_INIT_POS;
    enemy.speed = ENEMY_INIT_SPEED;
    enemy.direction = LEFT;

    while (1)  {

        // update position, use wraparound logic
        if (enemy.direction == LEFT) {
            enemy.position = enemy.position << 1;
            if (enemy.position == 0) enemy.position = 0x0001;
        }
        else {
            enemy.position = enemy.position >> 1;
            if (enemy.position == 0) enemy.position = 0x8000;
        }

        // write enemy to the screen
        WriteLeds(ENEMY_COLOR, enemy.position);

        // sleep for speed amount of time
        G8RTOS_Sleep(enemy.speed);
    }
}

/* Runs every second, printing out relevant variables, waking up coins and
 * potentially the player, and modifying the direction of the enemy. */
void quizPThread(void) {

    // print variables
    BackChannelPrintIntVariable("player points", player.points);
    BackChannelPrintIntVariable("times enemy caught player", enemy.times_caught_player);

    // wake up coins
    for (int i = 0; i < MAX_COINS; ++i) {
        if (!coins[i].awake) {
            if (coins[i].sleep_count <= 0) coins[i].awake = true;
            else coins[i].sleep_count -= 1;
        }
    }

    // flip enemy direction if necessary
    if (enemy.flip_count <= 0) {
        if (enemy.direction == LEFT) enemy.direction = RIGHT;
        else enemy.direction = LEFT;
        enemy.flip_count = LENGTH_ENEMY_FLIP_COUNT;
    }
    else enemy.flip_count -= 1;

    // wake up the player if necessary
    if (!player.awake) {
        if (player.sleep_count <= 0) player.awake = true;
        else player.sleep_count -= 1;
    }
}
