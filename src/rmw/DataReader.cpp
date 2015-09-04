#include "DataReader.h"
#include "rmw.h"

DataReader::DataReader(const char* topic, const char* callerID, const char* md5sum, const char* msgType)
{
    strcpy(this->topic, topic);
    strcpy(this->callerID, callerID);
    strcpy(this->md5sum, md5sum);
    strcpy(this->msgType, msgType);
    qHandle = xQueueCreate(6, RX_QUEUE_MSG_SIZE);
    connectionID = 0;
    // TODO: make a unique task name
    xTaskCreate(task, (const signed char*)"topic", 150, (void*)this, tskIDLE_PRIORITY + 2, NULL);
}

void DataReader::addCallback(void(*callback)(void* data, void* obj), void* obj)
{
    static int lastIndex = 0;
    if (lastIndex<MAX_CALLBACKS)
    {
        callbacks[lastIndex] = callback;
        objects[lastIndex] = obj;
        lastIndex++;
    }
}

void DataReader::task(void* arg)
{
    DataReader* self = (DataReader*) arg;
    unsigned char data[RX_QUEUE_MSG_SIZE];
    for (;;)
    {
        os_printf("dr task\n");
        // Try to receive message, put the task to sleep for at most RXTIMEOUT ticks if queue is empty.
        if (xQueueReceive(self->qHandle, data, RXTIMEOUT))
        {
            os_printf("%s received msg\n", self->callerID);
            for (int i=0; i< MAX_CALLBACKS; i++)
            {
                if (self->callbacks[i] != NULL)
                    self->callbacks[i]((void*)&data[4], self->objects[i]);
            }
        }
    }
}

const char* DataReader::getTopic()
{
    return topic;
}

const char* DataReader::getCallerID()
{
    return callerID;
}

const char* DataReader::getMd5Sum()
{
    return md5sum;
}

const char* DataReader::getMsgType()
{
    return msgType;
}

uint32_t DataReader::getConnectionID()
{
    return connectionID;
}

void DataReader::setConnectionID(uint32_t connectionID)
{
    this->connectionID = connectionID;
}

void DataReader::enqueueMessage(const char* msg)
{
    // Initialize memory (in stack) for message.
    unsigned char data[RX_QUEUE_MSG_SIZE];
    // Copy message into the previously initialized memory.
    memcpy(data, msg, RX_QUEUE_MSG_SIZE);
    // Try to send message if queue is non-full.
    // TODO: Check if we are still "connected" to the end point. (i.e. the node at the remote end is still running)
    if (qHandle)
    {
        if (xQueueSend(qHandle, &data, 0))
        {

        }
    }
}
