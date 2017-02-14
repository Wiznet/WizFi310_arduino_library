#include "WizFi310.h"
#include "WizFi310Udp.h"

#include "utility/WizFi310Drv.h"
#include "utility/Debug.h"

/* Constructor */
WiFiUDP::WiFiUDP() : _sock(NO_SOCKET_AVAIL)
{
}

/* Start WiFiUDP socket, listening at local port PORT */

uint8_t WiFiUDP::begin(uint16_t port)
{
    uint8_t sock = WizFi310Drv::getFirstSocket();
    if ( sock != NO_SOCKET_AVAIL )
    {
        if( WizFi310Drv::startUdpServer(sock, port) == false )
        {
            return 0;
        }

        _sock = sock;
        _port = port;
        WizFi310Drv::_state[_sock] = _sock;
        WizFi310Drv::_localPort = _port;
    }

    return 1;
}

/* return number of bytes available in the current packet,
   will return zero if parsePacket hasn't been called yet */
int WiFiUDP::available()
{
     if (_sock != NO_SOCKET_AVAIL)
     {
        int bytes = WizFi310Drv::availData();
        if (bytes>0)
        {
            return bytes;
        }
    }

    return 0;
}

/* Release any resources being used by this WiFiUDP instance */
void WiFiUDP::stop()
{
      if (_sock == NO_SOCKET_AVAIL)
        return;

      WizFi310Drv::stopClient(_sock);  
      WizFi310Drv::_state[_sock] = NA_STATE;
      _sock = NO_SOCKET_AVAIL;
      _remotePort = 0;
      _port = 0;
}

int WiFiUDP::beginPacket(const char *host, uint16_t port)
{
    if (_sock != NO_SOCKET_AVAIL)
    {
        //LOGDEBUG("socket is already used");
        //LOGDEBUG1("_sock :",_sock);
        stop();
    }

    uint8_t sock = WizFi310Drv::getFirstSocket();
    if( WizFi310Drv::startClient(host, port, sock, UDP_MODE) == false )
    {
        return 0;
    }

    _sock = sock;
    _remotePort = port;
    WizFi310Drv::_state[_sock] = _sock;
    strcpy(_remoteHost, host);

    //LOGDEBUG1("_sock :",_sock);

    return 1;
}

int WiFiUDP::beginPacket(IPAddress ip, uint16_t port)
{
    char s[18];
    sprintf(s, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    return beginPacket(s, port);
}


int WiFiUDP::endPacket()
{
    timedAvailable();
    return 1; //ServerDrv::sendUdpData(_sock);
}

size_t WiFiUDP::write(uint8_t byte)
{
    if( available() > 0 )
        return available();

    return write(&byte, 1);
}

size_t WiFiUDP::write(const uint8_t *buffer, size_t size)
{
    if( available() > 0 || WizFi310Drv::m_esc_state )
        return available();

    bool r = WizFi310Drv::sendData(_sock, buffer, size);
    if (!r)
    {
        return 0;
    }

    return size;
}

int WiFiUDP::parsePacket()
{
    return available();
}

int WiFiUDP::read()
{
    uint8_t b=0;
    if (!available())
        return -1;

    bool connClose = false;
    WizFi310Drv::getData(_sock, &b, false, &connClose);

    return b;
}

int WiFiUDP::read(uint8_t* buf, size_t size)
{
    int i,c;

    for(i=0;i<size;i++)
    {
        c = timedRead();
        if( c < 0 ) return i;
        buf[i] = (uint8_t)c;
    }

    return i;
}

int WiFiUDP::peek()
{
  uint8_t b;
  if (!available())
    return -1;

  return b;
}

void WiFiUDP::flush()
{
  // TODO: a real check to ensure transmission has been completed
}


IPAddress  WiFiUDP::remoteIP()
{
    IPAddress ret;
    WizFi310Drv::getRemoteIpAddress(ret);
    return ret;
}

uint16_t  WiFiUDP::remotePort()
{
    return WizFi310Drv::getRemotePort();
}

////////////////////////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////////////////////////
int WiFiUDP::timedRead()
{
    int c;
    unsigned long startMillis;

    startMillis = millis();
    do {
      c = read();
      if (c >= 0) return c;
    } while(millis() - startMillis < 1000);
    return -1;     // -1 indicates timeout
}

int WiFiUDP::timedAvailable()
{
    unsigned long startMillis;

    startMillis = millis();
    do {
        available();
    } while(millis() - startMillis < 1000);
    return 1;
}

