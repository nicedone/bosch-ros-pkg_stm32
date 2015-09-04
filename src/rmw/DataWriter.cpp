#include "DataWriter.h"
#include "rmw.h"
#include "device_config.h"

DataWriter::DataWriter(const char* topic)
{
    strcpy(this->topic, topic);
    qHandle = xQueueCreate(QUEUE_LEN, QUEUE_MSG_SIZE);
    lastConnectionID = -1;
#ifdef UDPROS
    conn = netconn_new( NETCONN_UDP );
    netconn_bind(conn, IP_ADDR_ANY, UDP_LOCAL_PORT);
#endif
    char taskName[32];
    sprintf(taskName, "%s_dw", topic);

    xTaskCreate(task, (const signed char*)taskName, 512, (void*)this, tskIDLE_PRIORITY + 2, NULL);
}

void DataWriter::publishMsg(const ros::Msg& msg)
{
    unsigned char stream[MSG_LEN];
    #ifdef UDPROS
    char msgHeader[8];
    static uint8_t counter = 1;

    // Serialize message
    uint32_t offset = msg.serialize(stream+sizeof(Connection)+sizeof(msgHeader)+sizeof(uint32_t));
    memcpy(stream+sizeof(Connection)+sizeof(msgHeader), &offset, sizeof(uint32_t));

    ipcSend((const char*)stream+sizeof(Connection)+sizeof(msgHeader), topic);

    if (lastConnectionID >=0)
    for (long connectionID = 0; connectionID <= lastConnectionID; connectionID++)
    {
        Connection conn;
        conn.ipaddr = inet_addr(ROS_MASTER_IP); // TODO: ip must be specified in the msg.
        conn.port = ports[connectionID];
        if (conn.port > 0)
        {
            memcpy(stream, &conn, sizeof(Connection));

            memcpy(&msgHeader[0], &connectionID, sizeof(uint32_t));
            msgHeader[4] = 0;
            msgHeader[5] = counter++;
            msgHeader[6] = 0x01;
            msgHeader[7] = 0;
            memcpy(stream+sizeof(Connection), msgHeader, sizeof(msgHeader));

            // Try to send message if queue is non-full.
            // TODO: Check if we are still "connected" to the end point. (i.e. the node at the remote end is still running)
            if (qHandle && xQueueSend(qHandle, &stream, 0))
            {

            }
        }
    }


    #else
    ipcSend(stream, topic);
    #endif


}

const char* DataWriter::getTopic()
{
    return topic;
}

void DataWriter::task(void* arg)
{
    DataWriter* self = (DataWriter*) arg;
    char data[RX_QUEUE_MSG_SIZE];
    for (;;)
    {
        // Try to receive message, put the task to sleep for at most RXTIMEOUT ticks if queue is empty.
        if (xQueueReceive(self->qHandle, data, RXTIMEOUT))
        {
            self->networkSend(data);
        }
    }
}

void DataWriter::networkSend(const char* msg)
{
    #ifdef UDPROS
    Connection connection;
    memcpy(&connection, msg, sizeof(Connection));
    uint32_t msgLen = *((uint32_t*) (msg+sizeof(Connection)+8)) +4 +8;

    struct ip_addr ip;
    ip.addr = connection.ipaddr;
    uint16_t port = connection.port;
    err_t err = netconn_connect(conn, &ip, port);
    os_printf("Connecting %s:%d, err:%d, msglen:%d\n", ip, port, err, msgLen);
    struct netbuf *buf = netbuf_new();
    void* data = netbuf_alloc(buf, msgLen); // Also deallocated with netbuf_delete(buf)

    memcpy (data, msg+sizeof (Connection), msgLen);

    err = netconn_send(conn, buf);
    netbuf_delete(buf);
    #endif
}

int32_t DataWriter::addConnection(uint16_t port)
{
    if (port < 1)
        return -1;

    if (lastConnectionID < MAX_DATAWRITER_CONNECTIONS -1)
    {
        lastConnectionID++;
        ports[lastConnectionID] = port;
        return lastConnectionID;
    }
    else
        os_printf("Max number of allowed data writer connections is %d!\n", MAX_DATAWRITER_CONNECTIONS);

    return -1;
}


void DataWriter::ipcSend(const char* msg, const char* topic)
{
    DataReader* dr = RMW::instance()->getDataReader(topic);
    dr->enqueueMessage(msg);
}

bool DataWriter::doesConnectionExist(uint16_t port)
{
    for(uint16_t i=0; i<MAX_DATAWRITER_CONNECTIONS; i++)
    {
        if (port > 0 && ports[i] == port)
            return true;
    }
    return false;
}

void DataWriter::deleteConnection(uint16_t port)
{
    for(uint16_t i=0; i<MAX_DATAWRITER_CONNECTIONS; i++)
    {
        if (port > 0 && ports[i] == port)
            ports[i] = 0;
    }
}
