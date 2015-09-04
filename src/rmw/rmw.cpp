#include "rmw.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "XMLRequest.h"
#include "device_config.h"
#include "xmlrpc.h"


RMW* RMW::_instance = NULL;



RMW::RMW()
{


}

void RMW::start()
{
    os_printf("Test RMW\n");


    xTaskCreate(tcptask, (const signed char*)"HTTPClient", 1300, this, tskIDLE_PRIORITY + 2, NULL);


    XMLServer* server = new XMLServer("HTTPServer", XMLRPC_PORT, xmlrpcReceiveCallback);
    isUDPReceiveTaskCreated = false;

    xTaskCreate(UDPreceive, (const signed char*)"UDPReceive", 256, NULL, tskIDLE_PRIORITY + 2, NULL);



}

void RMW::tcptask(void* p)
{
    static uint16_t port = 30000;
    TCPData tcpData;

    RMW* self = RMW::instance();
    os_printf("HTTPClient\n");

    self->TCPqHandle = xQueueCreate(1, sizeof(TCPData));

    for(;;)
    {
        processQueue:
        if (!xQueueReceive(self->TCPqHandle, &tcpData, 65000))
        {
            goto processQueue;
        }
        else
        {
            os_printf("Q recv\n");
        port++;
        struct netconn *conn = netconn_new(NETCONN_TCP);
          err_t err;
        if (conn!=NULL) {
            // Bind connection to the specified number
            os_printf("Binding port %d\n", port);
            err = netconn_bind(conn, NULL, port);

              if (err == ERR_OK)
              {
                  struct ip_addr ip;
                  ip.addr = tcpData.serverIP;
                  os_printf("Connecting port %d\n", tcpData.serverPort);
                  err = netconn_connect (conn, &ip, tcpData.serverPort);

                  if (err == ERR_OK)
                  {
                      os_printf("Writing data!\n");
                      netconn_write(conn, tcpData.data, TCP_DATA_SIZE, NETCONN_COPY);

                          struct netbuf *buf;
                          char *data;
                          u16_t len;
                          uint32_t offset = 0;
                          if ((buf = netconn_recv(conn)) != NULL) {
                            do {
                              netbuf_data(buf, (void**)&data, &len);
                              memcpy(self->rxBuffer+offset, data, len);

                              offset +=len;
                              os_printf("Netconn received %d bytes\n", len);


                            } while (netbuf_next(buf) >= 0);
                            //self->onReceive(tcpData.receiveCallback, tcpData.obj, self->rxBuffer);
                              os_printf("Received %d bytes!\n", strlen(data));
                              if (tcpData.receiveCallback != NULL)
                              {
                                  if (tcpData.obj != NULL)
                                      tcpData.receiveCallback(tcpData.obj, self->rxBuffer);
                              }

                              netbuf_delete(buf);
                          }
                  }
              }

          }
        netconn_close (conn );
        netconn_delete (conn );
        }
    }

    vTaskDelete(NULL);
}

void RMW::UDPreceive(void* params)
{
    struct netconn *conn;
    struct netbuf *buf;
    err_t err;

    // Initialize memory (in stack) for message.
    char message[60];
    os_printf("Test!\n");
    conn = netconn_new(NETCONN_UDP);
    for(;;)
    {
        // Check if connection was created successfully.
        if (conn!= NULL)
        {
            err = netconn_bind(conn, IP_ADDR_ANY, UDP_RECEIVE_PORT);

            // Check if we were able to bind to port.
            if (err == ERR_OK)
            {
                portTickType xLastWakeTime;
                // Initialize the xLastWakeTime variable with the current time.
                xLastWakeTime = xTaskGetTickCount();

                // Start periodic loop.
                while (1)
                {
                    buf = netconn_recv(conn);
                    if (buf!= NULL)
                    {
                        struct ip_addr* ip;
                        uint16_t port;
                        ip = buf->addr;
                        port = buf->port;
                        //if(ip != NULL)
                        //os_printf("Received from %d:%d!\n", ip->addr, port);
                        // Copy received data into message.
                        uint16_t len = netbuf_len(buf);
                        if (len>15)
                        {
                            netbuf_copy (buf, &message, len);
                            uint32_t connectionID = *((uint32_t*)&message[0]);
                            DataReader* dr = instance()->getDataReader(connectionID);
                            if (dr != NULL)
                            {
                                dr->enqueueMessage(&message[8]);
                                //os_printf("ConnectionID: %d, topic:%s\n", connectionID, tr->getTopic());
                            }
                        }
                        // Deallocate previously created memory.
                        netbuf_delete(buf);
                    }
                    // Use delay until to guarantee periodic execution of each loop iteration.
                    else
                    {
                        os_printf("buf = NULL!\n");
                        vTaskDelayUntil(&xLastWakeTime, 30);
                    }
                }
            }
            else
            {
                os_printf("cannot bind netconn\n");
            }
        }
        else
        {
            os_printf("cannot create new UDP netconn\n");
            conn = netconn_new(NETCONN_UDP);
        }
        // If connection failed, wait for 50 ms before retrying.
        vTaskDelay(50);
    }
}

void RMW::xmlrpcReceiveCallback(const char* data, char* buffer)
{
    os_printf("Receive callback, buffer addr: %08x!\n", buffer);
    //char mystr[] = "test str";
    /*char mystr[] = "HTTP/1.0 200 OK"
                                   "Server: BaseHTTP/0.3 Python/2.7.6"
                                   "Date: Sat, 01 January 1970 00:00:00 GMT"
                                   "Content-type: text/xml"
                                   "Content-length: 1000\n\n"
                                   "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><i4>1</i4></value><value></value><value><array><data><value>UDPROS</value><value>192.168.2.1</value><value><i4>44000</i4></value><value><i4>1</i4></value><value><i4>1500</i4></value><value><base64>EAAAAGNhbGxlcmlkPS90YWxrZXInAAAAbWQ1c3VtPTk5MmNlOGExNjg3Y2VjOGM4YmQ4ODNlYzczY2E0MWQxHwAAAG1lc3NhZ2VfZGVmaW5pdGlvbj1zdHJpbmcgZGF0YQoOAAAAdG9waWM9L2NoYXR0ZXIUAAAAdHlwZT1zdGRfbXNncy9TdHJpbmc=</base64></value></data></array></value></data></array></value></param></params></methodResponse>";



    strcpy(buffer, mystr);
    return;*/


    char methodName[48];
    {
        char* pos = strstr((char*)data, "<methodName>");
        char* pos2 = strstr((char*)data, "</methodName>");
        if (pos2 > pos)
        {

            strncpy (methodName, pos+12, pos2-pos-12);
            methodName[pos2-pos-12] = 0;
        }
    }

    os_printf("name:%s\n",methodName);

    os_printf("Strlen:%d\n",strlen(data));

    if (!strcmp(methodName, "requestTopic"))
    {
        char* pos = strstr(data, "<i4>");

        char* pos2 = strstr(data, "</i4>");
        os_printf("pos:%d, pos2:%d\n", pos, pos2);

        if (pos < pos2)
        {
            char portStr[pos2-pos-3];
            strncpy (portStr, pos+4, pos2-pos-4);
            portStr[pos2-pos-4] = 0;
            uint16_t port = atoi(portStr);
            os_printf("Port: %d\n",port);

            char* pos3 = strstr((char*)data, "</value></param><param><value>/");
            char* pos4;
            int len = strlen("</value></param><param><value>/");

            if (pos3)
            {
                pos4 = strstr((char*)pos3+len, "</value>");
                if (pos4 > pos3)
                {
                    char topic[pos4-pos3-len+1];
                    strncpy (topic, pos3+len, pos4-pos3-len);
                    topic[pos4-pos3-len] = 0;
                    os_printf("topic: %s\n", topic);

                    // TODO: Move UDPConnection to registerPublishers. Then extract topic name from data. Afterwards, find the corresponding connection.
                    DataWriter* dw = instance()->getDataWriter(topic);
                    if (dw != NULL)
                    {
                        int16_t connectionID = dw->addConnection(port);
                        if (connectionID > -1)
                        {
                            os_printf("Connection ID: %d\n", connectionID);
                            XMLRequest* response = new TopicResponse(THIS_REMOTE_IP, UDP_LOCAL_PORT, connectionID);
                            strcpy(buffer, response->getData());
                        }
                    }
                }
            }
        }
    }
    /*
        else if (!strcmp(methodName, "publisherUpdate"))
        {
            os_printf("Publisher Update!\n");
            char* pos = strstr(data, "<value><string>/master</string></value>");
            if (pos != 0)
            {
                char topic[MAX_TOPIC_LEN];
                while(1)
                {
                    char* pos2 = strstr((char*)pos, "<value><string>");
                    char* pos3 = strstr((char*)pos2, "</string></value>");
                    //os_printf("_pos:%d, _pos2:%d\n", pos2, pos3);
                    if (pos2 == NULL || pos3 == NULL)
                        break;
                    if (pos3 > pos2)
                    {
                        int offset = strlen("<value><string>");
                        char uri[pos3-pos2-offset+1];
                        strncpy (uri, pos2+offset, pos3-pos2-offset);
                        uri[pos3-pos2-offset] = 0;

                        if (!strcmp(uri, "/master"))
                        {
                        }
                        else if(uri[0] == '/')
                        {
                            strcpy(topic, &uri[1]);
                        }
                        else
                        {
                            uint16_t port;
                            char ip[32];
                            extractURI(uri, ip, &port);
                            if (strcmp(ip, THIS_REMOTE_IP))
                            {
                                os_printf("Topic:%s URI: %s:::%s:::%d\n", topic, uri, ip, port);
                                TopicReader* tr = getTopicReader(topic);
                                if (tr != NULL)
                                {
                                    if (!strcmp(ip, "SI-Z0M81"))
                                        strcpy(ip, ROS_MASTER_IP);

                                    tr->requestTopic(ip, port);
                                }
                            }
                        }
                    }
                    pos = pos3;
                }
            }
            else
                os_printf("pos is NULL\n");

        }*/
}

DataReader* RMW::getDataReader(const char* topic)
{
    for(uint16_t i=0; i<MAX_DATAREADERS;i++)
    {
        if (dataReaders[i] != NULL)
        {
            DataReader* dr = dataReaders[i];
            os_printf("Data reader %d: %s\n", i, dr->getCallerID());
            if (!strcmp(dr->getTopic(), topic))
                return dr;
        }
    }

    return NULL;
}

DataReader* RMW::getDataReader(const uint32_t connectionID)
{
    for(uint16_t i=0; i<MAX_DATAREADERS;i++)
    {
        if (dataReaders[i] != NULL)
        {
            DataReader* dr = dataReaders[i];
            if (dr->getConnectionID() == connectionID)
            {
                return dr;
            }
        }
    }
    return NULL;
}

DataWriter* RMW::getDataWriter(const char* topic)
{
    for(uint16_t i=0; i<MAX_DATAWRITERS;i++)
    {
        if (dataWriters[i] != NULL)
        {
            DataWriter* dw = dataWriters[i];
            if (!strcmp(dw->getTopic(), topic))
                return dw;
        }
    }
    return NULL;
}

DataWriter* RMW::getDataWriter(const uint16_t port)
{
    for(uint16_t i=0; i<MAX_DATAWRITERS;i++)
    {
        if (dataWriters[i] != NULL)
        {
            DataWriter* dw = dataWriters[i];
            if (dw)
            {
                if(dw->doesConnectionExist(port))
                {
                    return dw;
                }
            }
        }
    }
    return NULL;
}



DataReader* RMW::addDataReader(const char* callerID, const char* topic, const char* md5sum, const char* msgType)
{
    static uint16_t lastIndex = 0;

    if (lastIndex >= MAX_DATAREADERS-1)
        return NULL;

    DataReader* dr = new DataReader(topic, callerID, md5sum, msgType);
    dataReaders[lastIndex++] = dr;

    XMLRequest* req = new RegisterRequest("registerSubscriber", ROS_MASTER_IP, callerID, topic, msgType);
    sendRequest(req->getData(), SERVER_PORT_NUM, connectPublishers, dr);

    return dr;
}

DataWriter* RMW::addDataWriter(const char* callerID, const char* topic, const char* msgType)
{
    static uint16_t lastIndex = 0;

    if (lastIndex >= MAX_DATAWRITERS-1)
        return NULL;

    DataWriter* dw = new DataWriter(topic);
    dataWriters[lastIndex] = dw;
    lastIndex++;

    XMLRequest* req = new RegisterRequest("registerPublisher", ROS_MASTER_IP, callerID, topic, msgType);
    sendRequest(req->getData(), SERVER_PORT_NUM, connectSubscribers, dw);

    return dw;
}

void RMW::sendRequest(const char* data, uint16_t port, void(*receiveCallback)(const void* obj, const char* data), void* obj)
{
    //HTTPClient::instance()->sendData(data, port, receiveCallback, obj);
    TCPData tcpData;
    memcpy(tcpData.data, data, TCP_DATA_SIZE);
    tcpData.serverIP = inet_addr(ROS_MASTER_IP);
    tcpData.serverPort = port;
    tcpData.receiveCallback = receiveCallback;
    tcpData.obj = obj;

    TCPEnqueue:
    if (instance()->TCPqHandle)
    {
        if (xQueueSend(instance()->TCPqHandle, &tcpData, 0))
            os_printf("Enqueueing data!\n");
        else
        {
            os_printf("Queue is full!\n");
            vTaskDelay(100);
            goto TCPEnqueue;
        }
    }
    else
    {
        vTaskDelay(100);
        goto TCPEnqueue;
    }

}

void RMW::connectSubscribers(const void* obj, const char* data)
{
    os_printf("Connect subscribers!\n");
    char* pos = strstr((char*)data, "as publisher of");
    if (pos != 0)
    {
        while(1)
        {
            char* pos2 = strstr((char*)pos, "<value><string>");
            char* pos3 = strstr((char*)pos2, "</string></value>");
            if (pos2 == NULL || pos3 == NULL)
                break;
            if (pos3 > pos2)
            {
                int offset = strlen("<value><string>");
                char uri[pos3-pos2-offset+1];
                strncpy (uri, pos2+offset, pos3-pos2-offset);
                uri[pos3-pos2-offset] = 0;
                uint16_t port;
                char ip[32];
                extractURI(uri, ip, &port);
                os_printf("URI: %s:::%d\n", ip, port);
                //os_printf("URI: %s\n", uri);
                // Check if this uri already exists in a "PublisherURIs" list.
                if (strcmp(ip, THIS_REMOTE_IP))  // TODO: replace this with a method to check if ip is not equal self ip
                {
                    DataWriter* self = (DataWriter*) obj;
                    // TODO: Send publisher update to each remote subscriber
                    XMLRequest* req = new PublisherUpdate(self->getTopic(), uri);
                    sendRequest(req->getData(), port);
                }
            }
            pos = pos3;
        }
    }
    else
        os_printf("pos is NULL\n");

}

void RMW::connectPublishers(const void* obj, const char* data)
{
    char text[100];
    char* pos = strstr((char*)data, "Subscribed to");
    if (pos != 0)
    {
        while(1)
        {
            char* pos2 = strstr((char*)pos, "<value><string>");
            char* pos3 = strstr((char*)pos2, "</string></value>");
            if (pos2 == NULL || pos3 == NULL)
                break;
            if (pos3 > pos2)
            {
                int offset = strlen("<value><string>");
                char uri[pos3-pos2-offset+1];
                strncpy (uri, pos2+offset, pos3-pos2-offset);
                uri[pos3-pos2-offset] = 0;
                uint16_t port;
                char ip[32];
                extractURI(uri, ip, &port);
                os_printf("URI: %s:::%d\n", ip, port);
                //os_printf("URI: %s\n", uri);
                // Check if this uri already exists in a "PublisherURIs" list.
                if (strcmp(ip, THIS_REMOTE_IP)) // TODO: replace this with a method to check if ip is not equal self ip
                {
                    DataReader* self = (DataReader*) obj;
                    requestTopic(self, ROS_MASTER_IP, port);
                }
            }
            pos = pos3;
        }
    }
    else
        os_printf("pos is NULL\n");

}

void RMW::requestTopic(DataReader* dr, const char* ip, uint16_t serverPort)
{
    XMLRequest* req = new TopicRequest("requestTopic", ROS_MASTER_IP, dr->getCallerID(), dr->getTopic(), dr->getMd5Sum(), dr->getMsgType());
    sendRequest(req->getData(), serverPort, onResponse, dr);
}

void RMW::onResponse(const void* obj,const char* data)
{
    char* pos0 = strstr((char*)data, "UDPROS");
    char* pos01 = strstr((char*)pos0, "<i4>");
    char* pos = strstr((char*)pos01+4, "<i4>");
    char* pos2 = strstr((char*)pos, "</i4>");
    if (pos2 > pos && pos != NULL)
    {
        int offset = strlen("<i4>");
        char connID[pos2-pos-offset+1];
        strncpy (connID, pos+offset, pos2-pos-offset);
        DataReader* self = (DataReader*) obj;
        self->setConnectionID(atoi(connID));
        os_printf("Connection ID: %d, topic:%s\n", self->getConnectionID(), self->getTopic());
    }
}

void RMW::extractURI(const char* uri, char* ip, uint16_t* port)
{
    char* p = strstr(uri, "://");
    if (p != NULL)
    {
        char* pos = strstr(p+3, ":");
        if (pos != NULL && ip!= NULL)
        {
            memcpy(ip, p+3, pos-p-3);
            ip[pos-p-3] = 0;

            char portStr[6];
            strcpy(portStr, pos+1);
            *port = atoi(portStr);
        }
    }
}



extern "C" void ICMP_callback(struct pbuf *p, struct netif *inp)
{
    if (*(((u8_t *)p->payload)+20) == 3) // Port unreachable
    {
        uint16_t port = ntohs(*(((u16_t *)(p->payload+50))));
        os_printf("icmp_input: type: %d port: %d\n", *(((u8_t *)p->payload)+20), ntohs(*(((u16_t *)(p->payload+50)))));
        //os_printf("icmp_input: %08x %08x\n", *(((u32_t *)p->payload)), *(((u32_t *)p->payload)+1));
        DataWriter* dw = RMW::instance()->getDataWriter(port);
        dw->deleteConnection(port);
    }
}
