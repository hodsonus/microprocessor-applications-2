/*
 * Game.h
 *
 *  Created on: Feb 27, 2017
 *      Author: danny
 */

#ifndef GAME_H_
#define GAME_H_

/*********************************************** Includes ********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "G8RTOS/G8RTOS.h"
#include "cc3100_usage.h"
#include "LCDLib.h"
/*********************************************** Includes ********************************************************************/

/*********************************************** Externs ********************************************************************/

semaphore_t LED_Mutex, LCD_Mutex, WiFi_Mutex, SpecificPlayerInfo_Mutex, GameState_Mutex;

/*********************************************** Externs ********************************************************************/

/*********************************************** Global Defines ********************************************************************/
#define MAX_NUM_OF_PLAYERS          2
#define MAX_NUM_OF_BALLS            8

// This game can actually be played with 4 players... a little bit more challenging, but doable! 
#define NUM_OF_PLAYERS_PLAYING      2

/* Value that is used to scale the raw Joystick values. */
#define JOYSTICK_SCALER             8192

/* Size of game arena */
#define ARENA_MIN_X                 40
#define ARENA_MAX_X                 280
#define ARENA_MIN_Y                 0
#define ARENA_MAX_Y                 240

/* Size of objects */
/* Note: LEN is always X-coordinate, WID is always Y-coordinate */
#define PADDLE_LEN                  64
#define PADDLE_LEN_D2               (PADDLE_LEN >> 1)
#define PADDLE_WID                  4
#define PADDLE_WID_D2               (PADDLE_WID >> 1)
#define BALL_SIZE                   4
#define BALL_SIZE_D2                (BALL_SIZE >> 1)
#define SCORE_LEN                   8
#define SCORE_WID                   16

/* Centers for paddles at the center of the sides */
#define PADDLE_X_CENTER             (MAX_SCREEN_X >> 1)

/* Edge limitations for player's center coordinate */
#define HORIZ_CENTER_MAX_PL         (ARENA_MAX_X - PADDLE_LEN_D2)
#define HORIZ_CENTER_MIN_PL         (ARENA_MIN_X + PADDLE_LEN_D2)

/* Constant enters of each player */
#define TOP_PLAYER_CENTER_Y         (ARENA_MIN_Y + PADDLE_WID_D2)
#define BOTTOM_PLAYER_CENTER_Y      (ARENA_MAX_Y - PADDLE_WID_D2)

/* Edge coordinates for paddles */
#define TOP_PADDLE_EDGE             (ARENA_MIN_Y + PADDLE_WID)
#define BOTTOM_PADDLE_EDGE          (ARENA_MAX_Y - PADDLE_WID)

/* Edge coordinates for the scores */
#define TOP_SCORE_MIN_X             (TOP_SCORE_MAX_X - SCORE_LEN)
#define TOP_SCORE_MAX_X             (ARENA_MIN_X - 1)
#define TOP_SCORE_MIN_Y             ARENA_MIN_Y
#define TOP_SCORE_MAX_Y             (TOP_SCORE_MIN_Y + SCORE_WID)

#define BOTTOM_SCORE_MIN_X          (BOTTOM_SCORE_MAX_X - SCORE_LEN)
#define BOTTOM_SCORE_MAX_X          (ARENA_MIN_X - 1)
#define BOTTOM_SCORE_MIN_Y          (BOTTOM_SCORE_MAX_Y - SCORE_WID)
#define BOTTOM_SCORE_MAX_Y          ARENA_MAX_Y

/* Amount of allowable space for collisions with the sides of paddles */
#define WIGGLE_ROOM                 2

/* Value for velocities from contact with paddles */
#define _1_3_PADDLE                 11

/* Defines for Minkowski Alg. for collision */
#define WIDTH_TOP_OR_BOTTOM         ((PADDLE_LEN + BALL_SIZE) >> 1) + WIGGLE_ROOM
#define HEIGHT_TOP_OR_BOTTOM        ((PADDLE_WID + BALL_SIZE) >> 1) + WIGGLE_ROOM

/* Edge limitations for ball's center coordinate */
#define HORIZ_CENTER_MAX_BALL       (ARENA_MAX_X - BALL_SIZE_D2)
#define HORIZ_CENTER_MIN_BALL       (ARENA_MIN_X + BALL_SIZE_D2)
#define VERT_CENTER_MAX_BALL        (ARENA_MAX_Y - BALL_SIZE_D2)
#define VERT_CENTER_MIN_BALL        (ARENA_MIN_Y + BALL_SIZE_D2)

/* Maximum ball velocity */
#define MAX_BALL_VELO               6

/* Background color - Black */
#define BACK_COLOR                  LCD_BLACK
#define INIT_BALL_COLOR             LCD_WHITE

/* Offset for printing player to avoid blips from left behind ball */
#define PRINT_OFFSET                10

/* Used as status LEDs for Wi-Fi */
#define BLUE_LED                    BIT2
#define RED_LED                     BIT0

/* Used in place of raw numbers for ease and consistency of changes. */
#define MAX_PRIO                    0
#define MIN_PRIO                    255
/* TODO - determine what the below thread priorities should be. */
#define MOVEBALL_PRIO               50
#define GENBALL_PRIO                50
#define DRAWOBJ_PRIO                50
#define JOYSTICK_PRIO               50
#define SENDDATA_PRIO               50
#define RECEIVEDATA_PRIO            50
#define MOVELED_PRIO                50

/* Enums for player colors */
typedef enum
{
    PLAYER_RED = LCD_RED,
    PLAYER_BLUE = LCD_BLUE
} playerColor;

/* Enums for player numbers */
typedef enum
{
    BOTTOM = 0,
    TOP = 1
} playerPosition;

/*********************************************** Global Defines ********************************************************************/

/*********************************************** Data Structures ********************************************************************/
/*********************************************** Data Structures ********************************************************************/
#pragma pack ( push, 1)
/*
 * Struct to be sent from the client to the host
 */
typedef struct
{
    uint32_t IP_address;
    int16_t displacement;
    uint8_t playerNumber;
    bool ready;
    bool joined;
    bool acknowledge;
} SpecificPlayerInfo_t;

/*
 * General player info to be used by both host and client
 * Client responsible for translation
 */
typedef struct
{
    int16_t currentCenter;
    uint16_t color;
    playerPosition position;
} GeneralPlayerInfo_t;

/*
 * Struct of all the balls, only changed by the host
 */
typedef struct
{
    int16_t currentCenterX;
    int16_t currentCenterY;
    int16_t x_velocity;
    int16_t y_velocity;
    uint16_t color;
    bool alive;
} Ball_t;

/*
 * Struct to be sent from the host to the client
 */
typedef struct
{
    SpecificPlayerInfo_t player;
    GeneralPlayerInfo_t players[MAX_NUM_OF_PLAYERS];
    Ball_t balls[MAX_NUM_OF_BALLS];
    uint16_t numberOfBalls;
    bool winner;
    bool gameDone;
    uint8_t LEDScores[MAX_NUM_OF_PLAYERS];
    uint8_t overallScores[MAX_NUM_OF_PLAYERS];
} GameState_t;
#pragma pack ( pop )

/*
 * Struct of all the previous ball locations, only changed by self for drawing!
 */
typedef struct
{
    int16_t CenterX;
    int16_t CenterY;
    bool alive;
} PrevBall_t;

/*
 * Struct of all the previous players locations, only changed by self for drawing
 */
typedef struct
{
    int16_t Center;
} PrevPlayer_t;
/*********************************************** Data Structures ********************************************************************/

/*********************************************** Global Variables ********************************************************************/
SpecificPlayerInfo_t clientInfo;
GameState_t gameState;
PrevBall_t prevBalls[MAX_NUM_OF_BALLS];
PrevPlayer_t prevPlayers[MAX_NUM_OF_PLAYERS];
/*********************************************** Global Variables ********************************************************************/

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame();

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost();

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost();

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient();

/*
 * End of game for the client
 */
void EndOfGameClient();

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame();

/*
 * Thread that sends game state to client
 */
void SendDataToClient();

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient();

/*
 * Generate Ball thread
 */
void GenerateBall();

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost();

/*
 * Thread to move a single ball
 */
void MoveBall();

/*
 * End of game for the host
 */
void EndOfGameHost();

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread();

/*
 * Thread to draw all the objects in the game
 */
void DrawObjects();

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs();

/*
 * Thread to set up game as host or client, depending on user input
 */
void HostVsClient();

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole();

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player);

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer);

/*
 * Draw a new ball on screen
 */
void DrawBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall);

/*
 * Delete a dead ball on screen
 */
void DeleteBallOnScreen(PrevBall_t * previousBall);

/*
 * Function updates ball position on screen
 */
// Note: I commented out the outColor variable because I could not find any use of it
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall/*, uint16_t outColor */);

/*
 * Function updates overall scores
 */
void UpdateOverallScore();

/*
 * Function updates LED scores
 */
void UpdateLEDScore();

/*
 * Initializes and prints initial game state
 */
void InitBoardState();

/*
 * Adds the common game threads - abstraction used to clean up the initialization of a new game.
 */
void AddCommonGameThreads();

/*
 * Adds the client's game threads - abstraction used to clean up the initialization of a new game.
 */
void AddClientGameThreads();

/*
 * Adds the host's game threads - abstraction used to clean up the initialization of a new game.
 */
void AddHostGameThreads();

/*
 * Updates a particular player's displacement, given it's SpecificPlayerInfo_t struct.
 * NOTE - MUST BE HOLDING THE GAMESTATE MUTEX AND/OR THE CORRESPONDING SpecificPlayerInfo MUTEX WHEN CALLING THIS FUNCTION
 */
void UpdatePlayerDisplacement(SpecificPlayerInfo_t *player);

/*********************************************** Public Functions *********************************************************************/


#endif /* GAME_H_ */
