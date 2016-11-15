#include "WizFi310.h"
#include "WizFi310Udp.h"

#include "utility/WizFi310Drv.h"
#include "utility/Debug.h"

/* Constructor */
WiFiUDP::WiFiUDP() : _sock(NO_SOCKET_AVAIL) {}

/* Start WiFiUDP socket, listening at local port PORT */

uint8_t WiFiUDP::begin(uint16_t port)
{
    uint8_t sock = getFirstSocket();
    if (sock != NO_SOCKET_AVAIL)
    {
        //WizFi310Drv::startUdpServer(sock, port);
        //WizFi310Class::_server_port[sock] = port;
        WizFi310Drv::_localPort = port;
        _sock = sock;
        _port = port;
        return 1;
    }

    return 0;

}


/* return number of bytes available in the current packet,
   will return zero if parsePacket hasn't been called yet */
int WiFiUDP::available()
{
	 if (_sock != NO_SOCKET_AVAIL)
	 {
		int bytes = WizFi310Drv::availData(_sock);
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
	  _sock = NO_SOCKET_AVAIL;
}

int WiFiUDP::beginPacket(const char *host, uint16_t port)
{
  if (_sock == NO_SOCKET_AVAIL)
  {
      _sock = getFirstSocket();
  }
  if (_sock != NO_SOCKET_AVAIL)
  {
      WizFi310Drv::startClient(host, port, _sock, UDP_MODE);
      _remotePort = port;
	  strcpy(_remoteHost, host);
	  WizFi310Drv::_state[_sock] = _sock;
	  return 1;
  }

  return 0;
}


int WiFiUDP::beginPacket(IPAddress ip, uint16_t port)
{
	char s[18];
	sprintf(s, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

	return beginPacket(s, port);
}


int WiFiUDP::endPacket()
{
	return 1; //ServerDrv::sendUdpData(_sock);
}

size_t WiFiUDP::write(uint8_t byte)
{
  return write(&byte, 1);
}

size_t WiFiUDP::write(const uint8_t *buffer, size_t size)
{
	//bool r = EspDrv::sendDataUdp(_sock, _remoteHost, _remotePort, buffer, size);
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
	uint8_t b;
	if (!available())
		return -1;

	bool connClose = false;
	WizFi310Drv::getData(_sock, &b, false, &connClose);

	return b;
}

int WiFiUDP::read(uint8_t* buf, size_t size)
{
	if (!available())
		return -1;
	return WizFi310Drv::getDataBuf(_sock, buf, size);
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

// TODO remove duplication with WiFiEspClient::getFirstSocket()

uint8_t WiFiUDP::getFirstSocket()
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

