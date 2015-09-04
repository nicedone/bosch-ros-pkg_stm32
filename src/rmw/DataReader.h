#ifndef RMW_DATAREADER_H_
#define RMW_DATAREADER_H_
#define MAX_TOPIC_LEN 48

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

class DataReader
{
	char topic[MAX_TOPIC_LEN];
    char callerID[MAX_TOPIC_LEN];
    char md5sum[MAX_TOPIC_LEN];
    char msgType[MAX_TOPIC_LEN];

    uint32_t connectionID;
	xQueueHandle qHandle;

	static const int RX_QUEUE_MSG_SIZE = 128;
	static const int MAX_CALLBACKS = 5;

	void(*callbacks[MAX_CALLBACKS])(void* data, void* obj);
	void* objects[MAX_CALLBACKS];

public:
    DataReader(const char* topic, const char* callerID, const char* md5sum, const char* msgType);

    void addCallback(void(*callback)(void* data, void* obj), void* obj);

    static void task(void* arg);

    const char* getTopic();
    const char* getCallerID();
    const char* getMd5Sum();
    const char* getMsgType();

    uint32_t getConnectionID();
    void setConnectionID(uint32_t connectionID);

    void enqueueMessage(const char* msg);
};


#endif /* RMW_DATAREADER_H_ */
