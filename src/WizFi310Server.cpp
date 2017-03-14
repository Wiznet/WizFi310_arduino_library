#include "WizFi310Server.h"

#include "utility/WizFi310Drv.h"
#include "utility/debug.h"


WiFiServer::WiFiServer(uint16_t port)
{
    m_port = port;
    m_client_sock = 255;
}

void WiFiServer::begin()
{
    LOGDEBUG(F("Starting server"));

    m_client_sock = WizFi310Drv::startServer(m_port);

    if (m_client_sock != SOCK_NOT_AVAIL)
    {
        m_started = true;
        LOGINFO1(F("Server started on port"), m_port);
        LOGINFO1(m_client_sock, F(" client will connect"));
    }
    else
    {
        LOGERROR(F("Server failed to start"));
    }
}

WiFiClient WiFiServer::available(byte* status)
{
    uint8_t sock;

    WizFi310Drv::availData();
    if( WizFi310Drv::_state[m_client_sock] != NA_STATE )
    {
        LOGINFO1(F("New client"), m_client_sock);
        WiFiClient client(m_client_sock);
        return client;
    }

    return WiFiClient(255);
}

uint8_t WiFiServer::status()
{
    return WizFi310Drv::getServerState(0);
}

size_t WiFiServer::write(uint8_t b)
{
    return write(&b, 1);
}

size_t WiFiServer::write(const uint8_t *buffer, size_t size)
{
    size_t n = 0;

    for (int sock = 0; sock < MAX_SOCK_NUM; sock++)
    {
        if (WizFi310Drv::_state[sock] != 0)
        {
            WiFiClient client(sock);
            n += client.write(buffer, size);
        }
    }
    return n;
}


uint8_t WiFiServer::getFirstSocket()
{
    for (int i = 0; i < MAX_SOCK_NUM; i++)
    {
      if (WizFi310Drv::_state[i] == NA_STATE)
      {
          return i;
      }
    }
    return SOCK_NOT_AVAIL;
}
