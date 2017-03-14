#ifndef WIZFI310_WIZFI310SERVER_H_ 
#define WIZFI310_WIZFI310SERVER_H_

#include <Server.h>
#include "WizFi310.h"


class WiFiClient;

class WiFiServer : public Server
{

public:
    WiFiServer(uint16_t port);


    /*
    * Gets a client that is connected to the server and has data available for reading.
    * The connection persists when the returned client object goes out of scope; you can close it by calling client.stop().
    * Returns a Client object; if no Client has data available for reading, this object will evaluate to false in an if-statement.
    */
    WiFiClient available(uint8_t* status = NULL);

    /*
    * Start the TCP server
    */
    void begin();

    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);

    uint8_t status();

    uint8_t getFirstSocket();

    using Print::write;


private:
    uint8_t  m_client_sock;
    uint16_t m_port;
    bool m_started;
};

#endif
