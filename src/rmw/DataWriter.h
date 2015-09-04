#ifndef RMW_DATAWRITER_H_
#define RMW_DATAWRITER_H_
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"
#define RXTIMEOUT 1000
#include <string.h>
#define MAX_TOPIC_LEN 48
#define MSG_LEN 400
#define QUEUE_LEN 5
#define QUEUE_MSG_SIZE MSG_LEN

#include "DataReader.h"

#define UDPROS

#ifdef UDPROS
#include "tcp.h"
#include <lwip/sockets.h>
#include "lwip/ip_addr.h"
#include "tcpip.h"
#include "api.h"
#define UDP_LOCAL_PORT 46552
#define MAX_DATAWRITER_CONNECTIONS 40
#endif

extern "C"
{
#include "ros.h"
}

#include "msg.h"

class DataWriter
{
    char topic[MAX_TOPIC_LEN];
	xQueueHandle qHandle;
    static const int RX_QUEUE_MSG_SIZE = MSG_LEN;

#ifdef UDPROS
    struct netconn* conn;
    uint16_t ports[MAX_DATAWRITER_CONNECTIONS];
    int16_t lastConnectionID;
    struct Connection
    {
        uint32_t ipaddr;
        uint16_t port;
    };

#else
    frudp_pub_t *g_pub;
#endif
public:
    DataWriter(const char* topic);
    void publishMsg(const ros::Msg& msg);

    const char* getTopic();

    static void task(void* arg);

    int32_t addConnection(uint16_t port);

    bool doesConnectionExist(uint16_t port);
    void deleteConnection(uint16_t port);

private:
    void networkSend(const char* data);
    void ipcSend(const char* msg, const char* topic);
};

#endif /* RMW_DATAWRITER_H_ */
