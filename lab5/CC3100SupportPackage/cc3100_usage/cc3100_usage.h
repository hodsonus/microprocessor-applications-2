/*
 * cc3100_usage.h
 *
 *  Created on: Feb 16, 2017
 *      Author: Danny
 */

#ifndef CC3100_USAGE_H_
#define CC3100_USAGE_H_

/********************** Includes *********************/
#include "simplelink.h"
#include "sl_common.h"
/********************** Includes *********************/

/*
 * Description of CC3100 Usage:
 *
 * TODO: fill this in
 *
 *
 */

/**************************** Defines ******************************/
/*
 * Determines whether player is the host or client
 */
typedef enum
{
    Client = 0,
    Host = 1
} playerType;

#define SHIDA

#ifdef JOHN
    /* IP addressed of server side socket.
     * Should be in long format, E.g: 0xc0a8010a == 192.168.1.10
     * Change host IP by
     * Change IP using "ifconfig | grep inet", can also be found in System Preferences -> Network -> TCP/IP -> Router)
     */
    #define HOST_IP_ADDR           0xC0A801AD               // IP address of server to connect to (my laptop)
    #define PORT_NUM               5001                     // Port number to be used (same on laptop and board)
    #define NO_OF_PACKETS          1                        // Number of packets to send out

    /* Static IP address for host (my board IP, need to change if first 3 numbers do not match the laptop's) */
    #define CONFIG_IP       SL_IPV4_VAL(192,168,1,226)
    /* Subnet Mask for the station */
    #define AP_MASK         SL_IPV4_VAL(255,255,255,0)
    /* Default Gateway address (IP address of the router, can be found in System Preferences -> Network -> TCP/IP -> Router) */
    #define AP_GATEWAY      SL_IPV4_VAL(192,168,1,1)
    /* DNS Server Address */
    #define AP_DNS          SL_IPV4_VAL(0,0,0,0)
    #define SL_STOP_TIMEOUT        0xFF
#endif

#ifdef SHIDA
    /* IP addressed of server side socket.
     * Should be in long format, E.g: 0xc0a8010a == 192.168.1.10
     */
    //192.168.254.21
    #define HOST_IP_ADDR           0xC0A8FE15               // IP address of server to connect to
    #define PORT_NUM               5001                     // Port number to be used
    #define NO_OF_PACKETS          1                        // Number of packets to send out

    /*
     * Static IP address for host
     */
    #define CONFIG_IP       SL_IPV4_VAL(192,168,254,122)       /* Static IP to be configured */
    #define AP_MASK         SL_IPV4_VAL(255,255,255,0)      /* Subnet Mask for the station */
    #define AP_GATEWAY      SL_IPV4_VAL(192,168,254,254)        /* Default Gateway address */
    #define AP_DNS          SL_IPV4_VAL(0,0,0,0)            /* DNS Server Address */
    #define SL_STOP_TIMEOUT        0xFF
#endif

/* Application specific status/error codes */
typedef enum{
    DEVICE_NOT_IN_STATION_MODE = -0x7D0,        /* Choosing this number to avoid overlap w/ host-driver's error codes */
    BSD_UDP_CLIENT_FAILED = DEVICE_NOT_IN_STATION_MODE - 1,

    STATUS_CODE_MAX = -0xBB8
}e_AppStatusCodes;

#define NOTHING_RECEIVED -1
/**************************** Defines ******************************/


/*********************** User Functions ************************/
void SendData(_u8 *data, _u32 IP, _u16 BUF_SIZE);
_i32 ReceiveData(_u8 *data, _u16 BUF_SIZE);
void initCC3100(playerType playerRole);
_u32 getLocalIP();
/*********************** User Functions ************************/


/*********************** Event Handlers ************************/
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock);
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent);
void SimpleLinkHttpServerCallback(SlHttpServerEvent_t *pHttpEvent,
                                  SlHttpServerResponse_t *pHttpResponse);
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent);
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent);
/*********************** Event Handlers ************************/



#endif /* CC3100_USAGE_H_ */
