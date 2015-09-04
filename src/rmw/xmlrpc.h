#ifndef RMW_XMLRPC_H_
#define RMW_XMLRPC_H_
#include <stdint.h>
#include "tcp.h"
#include <lwip/sockets.h>
#include "lwip/ip_addr.h"
#include "tcpip.h"
#include "api.h"

extern "C"
{
#include "ros.h"
}

#include "XMLRequest.h"

#define TOPIC_COUNT 20
#define MAX_TOPIC_LEN 48


#include "netconf.h"

#define TCP_DATA_SIZE 1200


#define HTTP_SERVER_BUFFER_SIZE 1500
class XMLServer
{
private:
        static void tcptask(void* arg)
        {
            XMLServer* self = (XMLServer*) arg;

            struct netconn *conn, *newconn;
            err_t err;
            // Create a new connection identifier.
            create_conn:
              conn = netconn_new(NETCONN_TCP);

            if (conn!=NULL) {
              // Bind connection to the specified port number.
              bind_conn:
              err = netconn_bind(conn, NULL, self->port);

              if (err == ERR_OK) {
                // Tell connection to go into listening mode.
                listen_conn:
                  err = netconn_listen(conn);

                //uint16_t bufferLength = 1500;
                //char buffer[bufferLength];
                  if (err != ERR_OK)
                  {
                      os_printf("Cannot listen conn!\n");
                      vTaskDelay(10);
                      goto listen_conn;
                  }
                  else
                while (1) {
                    os_printf("netconn accept!!\n");
                  // Grab new connection.
                  newconn = netconn_accept(conn);
                  os_printf("netconn accepted, newconn: 0x%08x!!\n", newconn);
                  // Process the new connection.
                  if (newconn) {
                      self->onAccept();
                    struct netbuf *buf;
                    char *data;
                    u16_t len;
                    uint32_t offset = 0;
                    if ((buf = netconn_recv(newconn)) != NULL) {
                      do {
                        netbuf_data(buf, (void**)&data, &len);
                        self->onReceive(data);
                        memcpy(self->rxBuffer+offset, data, len);
                        offset +=len;
                        os_printf("Netconn received %d bytes\n", len);


                      } while (netbuf_next(buf) >= 0);
                      //self->onReceive(self->rxBuffer);
                      self->receiveCallback(self->rxBuffer, self->buffer);
                      /*if (newconn && self->buffer && strlen(self->buffer) > 0)
                      {
                          for (int i=0; i<strlen(self->buffer) / 500 + 1; i++)
                            netconn_write(newconn, &self->buffer[i*500],
                                    strlen(&self->buffer[i*500]) < 500 ? strlen(&self->buffer[i*500]) : 500,
                                    NETCONN_NOCOPY);
                      }
                      else
                          os_printf("self buffer or newconn is NULL!\n");*/
                      //netconn_write(newconn, self->buffer, strlen(self->buffer), NETCONN_NOCOPY);
                      //netconn_write(newconn, mydata, strlen(mydata), NETCONN_NOCOPY);

                        //char test[] = "small string!";
                        char test[] = "HTTP/1.0 200 OK"
                                "Server: BaseHTTP/0.3 Python/2.7.6"
                                "Date: Sat, 01 January 1970 00:00:00 GMT"
                                "Content-type: text/xml"
                                "Content-length: 1000\n\n"
                                "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><i4>1</i4></value><value></value><value><array><data><value>UDPROS</value><value>192.168.2.1</value><value><i4>44000</i4></value><value><i4>1</i4></value><value><i4>1500</i4></value><value><base64>EAAAAGNhbGxlcmlkPS90YWxrZXInAAAAbWQ1c3VtPTk5MmNlOGExNjg3Y2VjOGM4YmQ4ODNlYzczY2E0MWQxHwAAAG1lc3NhZ2VfZGVmaW5pdGlvbj1zdHJpbmcgZGF0YQoOAAAAdG9waWM9L2NoYXR0ZXIUAAAAdHlwZT1zdGRfbXNncy9TdHJpbmc=</base64></value></data></array></value></data></array></value></param></params></methodResponse>";

                        os_printf("netconn writing %d bytes....\n", strlen(test));
                        //netconn_write(newconn, test, strlen(test), NETCONN_COPY);

                        strcpy(self->rxBuffer, self->buffer);

                        netconn_write(newconn, self->rxBuffer, strlen(self->rxBuffer), NETCONN_NOCOPY);


                      netbuf_delete(buf);
                    }

                    // Close connection and discard connection identifier.
                    netconn_close(newconn);

                    netconn_delete(newconn);
                  }
                }
              } else {
                os_printf(" can not bind TCP netconn\n");
                vTaskDelay(10);
                goto bind_conn;
              }
            } else {
                os_printf("can not create TCP netconn\n");
                vTaskDelay(10);
                goto create_conn;
            }


          vTaskDelete(NULL);
        }
        void onAccept()
        {
            os_printf("Accept, port:%d!\n", port);
            uint16_t remainingHeap = xPortGetFreeHeapSize();
            os_printf("on accept: remaining heap: %dB!\n", remainingHeap/10);
        }
        void onReceive(const char* data)
        {
            os_printf("Received %d bytes!\n", strlen(data));
        }
        void onSendAcknowledged()
        {
            os_printf("Sent data!\n");
        }
        char rxBuffer[1500];
        char buffer[1500];
        uint16_t port;
        void(*receiveCallback)(const char* data, char* buffer);
public:
    XMLServer(const char* taskName, uint16_t port, void(*receiveCallback)(const char* data, char* buffer) = NULL)
    {
        this->port = port;
        this->receiveCallback = receiveCallback;
        //xTaskCreate(tcptask, (const signed char*)taskName, 1600, this, tskIDLE_PRIORITY + 2, NULL);
        sys_thread_new((char*)taskName, tcptask, this, 1600, tskIDLE_PRIORITY + 2);
    }

};





#endif /* RMW_XMLRPC_H_ */
