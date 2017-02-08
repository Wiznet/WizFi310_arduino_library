/*
 * Copyright (C) WIZnet, Inc. All rights reserved.
 * Use is subject to license terms.
 */
#include <inttypes.h>
#include "WizFi310.h"
#include "WizFi310Client.h"

#include "utility/WizFi310Drv.h"
#include "utility/Debug.h"

WiFiClient::WiFiClient() : _sock(255)
{
}

WiFiClient::WiFiClient(uint8_t sock) : _sock(sock)
{
}

int WiFiClient::connectSSL(const char* host, uint16_t port)
{
    return connect(host, port, SSL_MODE);
}

int WiFiClient::connectSSL(IPAddress ip, uint16_t port)
{
    char s[16];
    sprintf_P(s,PSTR("%d.%d.%d.%d"), ip[0],ip[1],ip[2],ip[3]);
    return connect(s, port, SSL_MODE);
}

int WiFiClient::connect(const char* host, uint16_t port)
{
    return connect(host, port, TCP_MODE);
}

int WiFiClient::connect(IPAddress ip, uint16_t port)
{
    char s[16];
    sprintf_P(s,PSTR("%d.%d.%d.%d"), ip[0],ip[1],ip[2],ip[3]);
    return connect(s, port, TCP_MODE);
}

int WiFiClient::connect(const char* host, uint16_t port, uint8_t protMode)
{
    LOGINFO1(F("Connecting to"), host);

    _sock = WizFi310Drv::getFirstSocket();

    if (_sock != NO_SOCKET_AVAIL)
    {
        if (!WizFi310Drv::startClient(host, port, _sock, protMode))
            return 0;

        WizFi310Drv::_state[_sock] = _sock;
    }
    else
    {
        LOGINFO(F("No socket available"));
        return 0;
    }
    return 1;
}

size_t WiFiClient::write(uint8_t b)
{
      return write(&b, 1);
}

size_t WiFiClient::write(const uint8_t *buf, size_t size)
{
    if (_sock >= MAX_SOCK_NUM or size==0)
    {
        setWriteError();
        return 0;
    }

    bool r = WizFi310Drv::sendData(_sock, buf, size);
    if (!r)
    {
        setWriteError();
        LOGERROR1(F("Failed to write to socket"), _sock);
        delay(4000);
        stop();
        return 0;
    }

    return size;
}

int WiFiClient::available()
{
    if (_sock != 255)
    {
        int bytes = WizFi310Drv::availData();
        if (bytes > 0)
        {
            return bytes;
        }
    }
    return 0;
}

int WiFiClient::read()
{
    uint8_t b;
    if (!available())
        return -1;

    bool connClose = false;
    if( WizFi310Drv::getData(_sock, &b, false, &connClose) == false )
        return -1;

    if (connClose)
    {
        WizFi310Drv::_state[_sock] = NA_STATE;
        _sock = 255;
    }

    return b;
}

int WiFiClient::read(uint8_t* buf, size_t size)
{
//    if (!available())
//        return -1;
    return WizFi310Drv::getDataBuf(_sock, buf, size);
}

int WiFiClient::peek()
{
    uint8_t b;
    if (!available())
        return -1;

    bool connClose = false;
    WizFi310Drv::getData(_sock, &b, true, &connClose);

    if (connClose)
    {
        WizFi310Drv::_state[_sock] = NA_STATE;
        _sock = 255;
    }

    return b;
}

void WiFiClient::flush()
{
    while (available())
        read();
}

void WiFiClient::stop()
{
    if (_sock == 255)
        return;

    LOGINFO1(F("Disconnecting "), _sock);

    WizFi310Drv::stopClient(_sock);

    WizFi310Drv::_state[_sock] = NA_STATE;
    _sock = 255;
}

uint8_t WiFiClient::connected()
{
    return (status() == ESTABLISHED);
}

WiFiClient::operator bool()
{
  return _sock != 255;
}

////////////////////////////////////////////////////////////////////////////////
// Additional WiFi standard methods
////////////////////////////////////////////////////////////////////////////////
uint8_t WiFiClient::status()
{
    if (_sock == 255)
    {
        return CLOSED;
    }

    if (WizFi310Drv::availData())
    {
        return ESTABLISHED;
    }

    if (WizFi310Drv::getClientState(_sock) == true )
    {
        return ESTABLISHED;
    }

    WizFi310Drv::_state[_sock] = NA_STATE;
    _sock = 255;

    return CLOSED;
}

IPAddress WiFiClient::remoteIP()
{
    IPAddress ret;
    WizFi310Drv::getRemoteIpAddress(ret);
    return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////////////////////////


size_t WiFiClient::printFSH(const __FlashStringHelper *ifsh, bool appendCrLf)
{
    size_t size = strlen_P((char*)ifsh);
    
    if (_sock >= MAX_SOCK_NUM or size==0)
    {
        setWriteError();
        return 0;
    }

    bool r = WizFi310Drv::sendData(_sock, ifsh, size, appendCrLf);
    if (!r)
    {
        setWriteError();
        LOGERROR1(F("Failed to write to socket"), _sock);
        delay(4000);
        stop();
        return 0;
    }

    return size;
}

