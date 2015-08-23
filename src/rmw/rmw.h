#ifndef RMW_RMW_H_
#define RMW_RMW_H_
#include <stdint.h>


#include "tcp.h"
#include <lwip/sockets.h>
#include "lwip/ip_addr.h"
#include "tcpip.h"
#include "api.h"

#define UDP_LOCAL_PORT 46552
#define UDP_RECEIVE_PORT 44100

extern "C"
{
#include "ros.h"
}

#include "DataReader.h"
#include "DataWriter.h"


#define MAX_DATAREADERS 10
#define MAX_DATAWRITERS 10

#define TCP_DATA_SIZE 1200

class RMW
{
    bool isUDPReceiveTaskCreated;
    DataReader* dataReaders[MAX_DATAREADERS];
    DataWriter* dataWriters[MAX_DATAWRITERS];
    RMW();

public:
    static RMW* _instance;

    static RMW *instance()
    {
        if (!_instance)
          _instance = new RMW();
        return _instance;

        os_printf("Test\n");
    }

    void start();

    DataReader* getDataReader(const char* topic);
    DataReader* getDataReader(const uint32_t connectionID);
    DataWriter* getDataWriter(const char* topic);
    DataWriter* getDataWriter(const uint16_t port);

    DataReader* addDataReader(const char* topic, const char* callerID, const char* md5sum, const char* msgType);
    DataWriter* addDataWriter(const char* topic, const char* callerID, const char* msgType);
private:
    static void UDPreceive(void* params);

    static void xmlrpcReceiveCallback(const char* data, char* buffer);





    static void sendRequest(const char* data, uint16_t port, void(*receiveCallback)(const void* obj, const char* data) = NULL, void* obj = NULL);

    static void connectSubscribers(const void* obj, const char* data);
    static void connectPublishers(const void* obj, const char* data);

    static void extractURI(const char* uri, char* ip, uint16_t* port);

    static void requestTopic(DataReader* dr, const char* ip, uint16_t serverPort);
    static void onResponse(const void* obj,const char* data);


    struct TCPData
    {
        uint16_t serverPort;
        uint32_t serverIP;
        char data[TCP_DATA_SIZE];
        void(*receiveCallback)(const void* obj, const char* data);
        void* obj;
    };

    xQueueHandle TCPqHandle;
    char rxBuffer[1024];
    static void tcptask(void* arg);
};


#endif /* RMW_RMW_H_ */
