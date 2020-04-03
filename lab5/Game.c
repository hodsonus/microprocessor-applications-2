/*
 * Game.c
 *
 *  Created on: Apr 3, 2020
 *      Author: John Hodson, Shida Yang
 */

#include "Game.h"

/*********************************************** Client Threads *********************************************************************/
/*
 * Thread for client to join game
 */
void JoinGame()
{
   // TODO
}

/*
 * Thread that receives game state packets from host
 */
void ReceiveDataFromHost()
{
   // TODO
}

/*
 * Thread that sends UDP packets to host
 */
void SendDataToHost()
{
   // TODO
}

/*
 * Thread to read client's joystick
 */
void ReadJoystickClient()
{
   // TODO
}

/*
 * End of game for the client
 */
void EndOfGameClient()
{
   // TODO
}

/*********************************************** Client Threads *********************************************************************/


/*********************************************** Host Threads *********************************************************************/
/*
 * Thread for the host to create a game
 */
void CreateGame()
{
   // TODO
}

/*
 * Thread that sends game state to client
 */
void SendDataToClient()
{
   // TODO
}

/*
 * Thread that receives UDP packets from client
 */
void ReceiveDataFromClient()
{
   // TODO
}

/*
 * Generate Ball thread
 */
void GenerateBall()
{
   // TODO
}

/*
 * Thread to read host's joystick
 */
void ReadJoystickHost()
{
   // TODO
}

/*
 * Thread to move a single ball
 */
void MoveBall()
{
   // TODO
}

/*
 * End of game for the host
 */
void EndOfGameHost()
{
   // TODO
}

/*********************************************** Host Threads *********************************************************************/


/*********************************************** Common Threads *********************************************************************/
/*
 * Idle thread
 */
void IdleThread()
{
   while(1);
}

/*
 * Thread to draw all the objects in the game
 */
void DrawObjects()
{
   // TODO
}

/*
 * Thread to update LEDs based on score
 */
void MoveLEDs()
{
   // TODO
}

/*
 * Thread to wait for player decision on Host vs. Client
 */
void HostVsClient()
{
    playerType role = GetPlayerRole();

    if (role == Client)
    {
        // TODO - initialize client threads
    }
    else
    {
        // TODO - initialize host threads
    }

    G8RTOS_KillSelf();
}

/*********************************************** Common Threads *********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Returns either Host or Client depending on button press
 */
playerType GetPlayerRole()
{
   // TODO
}

/*
 * Draw players given center X center coordinate
 */
void DrawPlayer(GeneralPlayerInfo_t * player)
{
   // TODO
}

/*
 * Updates player's paddle based on current and new center
 */
void UpdatePlayerOnScreen(PrevPlayer_t * prevPlayerIn, GeneralPlayerInfo_t * outPlayer)
{
   // TODO
}

/*
 * Function updates ball position on screen
 */
void UpdateBallOnScreen(PrevBall_t * previousBall, Ball_t * currentBall, uint16_t outColor)
{
   // TODO
}

/*
 * Initializes and prints initial game state
 */
void InitBoardState()
{
   // TODO
}

/*********************************************** Public Functions *********************************************************************/
