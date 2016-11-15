#include "WizFi310Server.h"

#include "utility/WizFi310Drv.h"
#include "utility/debug.h"


WiFiServer::WiFiServer(uint16_t port)
{
	_port = port;
}

void WiFiServer::begin()
{
	LOGDEBUG(F("Starting server"));

	_started = WizFi310Drv::startServer(_port);

	if (_started)
	{
		LOGINFO1(F("Server started on port"), _port);
	}
	else
	{
		LOGERROR(F("Server failed to start"));
	}
}

WiFiClient WiFiServer::available(byte* status)
{
	// TODO the original method seems to handle automatic server restart

	int bytes = WizFi310Drv::availData(0);
	if (bytes>0)
	{
		LOGINFO1(F("New client"), WizFi310Drv::_connId);
		WiFiClient client(WizFi310Drv::_connId);
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
