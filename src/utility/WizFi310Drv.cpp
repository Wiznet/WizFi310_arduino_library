/*--------------------------------------------------------------------
This file is part of the Arduino WiFiEsp library.

The Arduino WiFiEsp library is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

The Arduino WiFiEsp library is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with The Arduino WiFiEsp library.  If not, see
<http://www.gnu.org/licenses/>.
--------------------------------------------------------------------*/

#include <Arduino.h>
#include <avr/pgmspace.h>
#include "debug.h"
#include "WizFi310Drv.h"
#include "general_parse.h"

#define NUM_WIZFI_TAGS TAG_NUM

// maximum size of AT command
#define CMD_BUFFER_SIZE 300

typedef enum
{
    TAG_ERROR = -1,
    TAG_OK = 0,
//  TAG_CONNECT,
//  TAG_DISCONNECT,
//    TAG_LINKUP,
//    TAG_LINKDOWN
    TAG_NUM
} TagsEnum;

const char* WIZFI310EVNT[]=
{
    "[DISCONNECT ",
    "[Link-Down Event]\r\n"
};

char WizFi310Drv::fwVersion[]    = {0};
char WizFi310Drv::_ssid[]        = {0};
uint8_t WizFi310Drv::_bssid[]    = {0};
uint8_t WizFi310Drv::_mac[]      = {0};
uint8_t WizFi310Drv::_localIp[]  = {0};
uint8_t WizFi310Drv::_remoteIP[] = {0};

long     WizFi310Drv::_bufPos=0;
uint8_t  WizFi310Drv::_connId=0;
uint16_t WizFi310Drv::_remotePort = 0;
uint8_t  WizFi310Drv::_remoteIp[] = {0};

uint16_t WizFi310Drv::_localPort = 5000;

uint16_t WizFi310Drv::m_esc_state = ESC_IDLE;
int      WizFi310Drv::m_recved_len = 0;


bool WizFi310Drv::m_use_dhcp=true;

int16_t WizFi310Drv::_state[MAX_SOCK_NUM] = { NA_STATE, NA_STATE, NA_STATE, NA_STATE,
                                              NA_STATE, NA_STATE, NA_STATE, NA_STATE };
Stream *WizFi310Drv::WizFi310Serial;
WizFiRingBuffer WizFi310Drv::ringBuf(CMD_BUFFER_SIZE);

WizFi310Drv::WizFi310Drv()
{
}

void WizFi310Drv::wifiDriverInit(Stream *wizfiSerial)
{
    LOGDEBUG(F("> wifiDriverInit"));
    WizFi310Drv::WizFi310Serial = wizfiSerial;

    bool initOK = false;
    for(int i=0; i<5; i++)
     {
         if (sendCmd(F("AT\r"),1000) == TAG_OK)
         {
             initOK = true;
             break;
         }
         delay(1000);
     }

    if (!initOK)
    {
        LOGERROR(F("Cannot initialize WizFi310 module"));
        delay(5000);
        return;
    }

    sendCmd(F("AT+MECHO=0\r"));
    sendCmd(F("AT+WLEAVE\r"));
}



void WizFi310Drv::reset()
{

}

bool WizFi310Drv::wifiConnect(const char*ssid, const char *passphrase)
{
    LOGDEBUG(F("> wifiConnect"));

    if( sendCmd(F("AT+WLEAVE\r"),1000 ) == -1 )                 return false;
    if( sendCmd(F("AT+WSET=0,%s\r"),1000, ssid) == -1 )         return false;
    if( sendCmd(F("AT+WSEC=0,,%s\r"),1000, passphrase) == -1 )  return false;
    if( m_use_dhcp )
    {
        if( sendCmd(F("AT+WNET=1\r"), 1000) == -1 ) return false;
    }
    if( sendCmd(F("AT+WJOIN\r"),30000 ) == -1 ) return false;

    LOGINFO1(F("Connected to"), ssid);
    return true;
}

bool WizFi310Drv::wifiStartAP(const char *ssid, const char *pwd, uint8_t channel, uint8_t encry)
{
    LOGDEBUG(F("> wifiStartAP"));
    char ch_enc[10]={0,};
    if      ( encry == WIZ_TYPE_NONE )      strcpy(ch_enc,"OPEN");
    else if ( encry == WIZ_TYPE_WEP  )      strcpy(ch_enc,"WEP");
    else if ( encry == WIZ_TYPE_WPA  )      strcpy(ch_enc,"WPA");
    else if ( encry == WIZ_TYPE_WPA_AES )   strcpy(ch_enc,"WPAAES");
    else if ( encry == WIZ_TYPE_WPA2_AES )  strcpy(ch_enc,"WPA2AES");
    else if ( encry == WIZ_TYPE_WPA2_TKIP ) strcpy(ch_enc,"WPA2TKIP");
    else if ( encry == WIZ_TYPE_WPA2_MIXED) strcpy(ch_enc,"WPA2");

    if( sendCmd(F("AT+WSET=1,%s\r"),1000, ssid) == -1 )             return false;
    if( sendCmd(F("AT+WSEC=1,%s,%s\r"),1000, ch_enc,pwd) == -1 )    return false;
    if( sendCmd(F("AT+WJOIN\r"),10000 ) == -1 )                     return false;

    LOGINFO1(F("Access point started"), ssid);
    return true;
}

int8_t WizFi310Drv::disconnect()
{
}

void WizFi310Drv::config(IPAddress ip, IPAddress subnet, IPAddress gw)
{
}

void WizFi310Drv::config(void)
{
}

uint8_t* WizFi310Drv::getMacAddress()
{
}

void WizFi310Drv::getIpAddress(IPAddress &ip)
{
    char buff[CMD_BUFFER_SIZE];
    char *token;
    char *p_ip;

    sendCmd(F("AT+WSTATUS\r"));
    getResponse(buff, sizeof(buff), 2);

    if( strstr(buff, "STA") || strstr(buff,"AP") )
    {
        // Parsing IP-Addr ( IF/SSID/IP-Addr/Gateway/MAC/TxPower(dBm)/RSSI(-dBm) )
        token = strtok(buff, "/");  token = strtok(NULL, "/");
        token = strtok(NULL, "/");

        p_ip = strtok(token,".");   _localIp[0] = atoi(p_ip);
        p_ip = strtok(NULL,".");    _localIp[1] = atoi(p_ip);
        p_ip = strtok(NULL,".");    _localIp[2] = atoi(p_ip);
        p_ip = strtok(NULL,".");    _localIp[3] = atoi(p_ip);
    }
    else
    {
        sendCmd(F("AT+WNET=?\r"));
        if( getResponse(buff, sizeof(buff), 1) > 0)
        {
            token = strtok(buff,",");   token = strtok(NULL,",");

            p_ip = strtok(token,".");   _localIp[0] = atoi(p_ip);
            p_ip = strtok(NULL,".");    _localIp[1] = atoi(p_ip);
            p_ip = strtok(NULL,".");    _localIp[2] = atoi(p_ip);
            p_ip = strtok(NULL,".");    _localIp[3] = atoi(p_ip);
        }
    }

    ip = _localIp;
}

void WizFi310Drv::getNetmask(IPAddress& mask)
{
    char buff[CMD_BUFFER_SIZE];
    char *token;
    char *p_ip;

    sendCmd(F("AT+WNET=?\r"));
    if( getResponse(buff, sizeof(buff), 1) > 0)
    {
        token = strtok(buff,",");   token = strtok(NULL,",");
        token = strtok(NULL,",");

        p_ip = strtok(token,".");   mask[0] = atoi(p_ip);
        p_ip = strtok(NULL,".");    mask[1] = atoi(p_ip);
        p_ip = strtok(NULL,".");    mask[2] = atoi(p_ip);
        p_ip = strtok(NULL,".");    mask[3] = atoi(p_ip);
    }
    else
    {
        mask[0] = 0;    mask[1] = 0;
        mask[2] = 0;    mask[3] = 0;
    }
}

void WizFi310Drv::getGateway(IPAddress& gw)
{
    char buff[CMD_BUFFER_SIZE];
    char *token;
    char *p_ip;

    sendCmd(F("AT+WSTATUS\r"));
    getResponse(buff, sizeof(buff), 2);

    if( strstr(buff, "STA") || strstr(buff,"AP") )
    {
        // Parsing Gateway ( IF/SSID/IP-Addr/Gateway/MAC/TxPower(dBm)/RSSI(-dBm) )
        token = strtok(buff, "/");  token = strtok(NULL, "/");
        token = strtok(NULL, "/");  token = strtok(NULL, "/");

        p_ip = strtok(token,".");   gw[0] = atoi(p_ip);
        p_ip = strtok(NULL,".");    gw[1] = atoi(p_ip);
        p_ip = strtok(NULL,".");    gw[2] = atoi(p_ip);
        p_ip = strtok(NULL,".");    gw[3] = atoi(p_ip);
    }
    else
    {
        sendCmd(F("AT+WNET=?\r"));
        if( getResponse(buff, sizeof(buff), 1) > 0)
        {
            token = strtok(buff,",");   token = strtok(NULL,",");
            token = strtok(NULL,",");   token = strtok(NULL,",");

            p_ip = strtok(token,".");   gw[0] = atoi(p_ip);
            p_ip = strtok(NULL,".");    gw[1] = atoi(p_ip);
            p_ip = strtok(NULL,".");    gw[2] = atoi(p_ip);
            p_ip = strtok(NULL,".");    gw[3] = atoi(p_ip);
        }
        else
        {
            gw[0] = 0;  gw[1] = 0;
            gw[2] = 0;  gw[3] = 0;
        }
    }
}

char* WizFi310Drv::getCurrentSSID()
{
    LOGDEBUG(F("> getCurrentSSID"));
    char buff[CMD_BUFFER_SIZE];
    char *token;

    memset(_ssid, 0, WL_SSID_MAX_LENGTH);

    sendCmd(F("AT+WSET=?\r"));
    if( getResponse(buff, sizeof(buff), 1) > 0)
    {
        token = strtok(buff,",");   token = strtok(NULL,",");
        memcpy(_ssid, token, WL_SSID_MAX_LENGTH);
    }

    return _ssid;
}

uint8_t* WizFi310Drv::getCurrentBSSID()
{
    LOGDEBUG(F("> getCurrentBSSID"));
    char buff[CMD_BUFFER_SIZE];
    char *token;
    char *p_bssid;

    memset(_bssid, 0, WL_MAC_ADDR_LENGTH);

    sendCmd(F("AT+WSET=?\r"));
    if( getResponse(buff, sizeof(buff), 1) > 0)
    {
        token = strtok(buff,",");   token = strtok(NULL,",");
        token = strtok(NULL,",");

        p_bssid = strtok(token, ":");
        _bssid[5] = (byte)strtol(p_bssid, NULL, 16);
        token = strtok(NULL, ":");
        _bssid[4] = (byte)strtol(p_bssid, NULL, 16);
        token = strtok(NULL, ":");
        _bssid[3] = (byte)strtol(p_bssid, NULL, 16);
        token = strtok(NULL, ":");
        _bssid[2] = (byte)strtol(p_bssid, NULL, 16);
        token = strtok(NULL, ":");
        _bssid[1] = (byte)strtol(p_bssid, NULL, 16);
        token = strtok(NULL, ":");
        _bssid[0] = (byte)strtol(p_bssid, NULL, 16);
    }

    return _bssid;
}

int32_t WizFi310Drv::getCurrentRSSI()
{
    LOGDEBUG(F("> getCurrentRSSI"));
    char buff[CMD_BUFFER_SIZE];
    char *token;
    int ret=0;

    sendCmd(F("AT+WSTATUS\r"));
    getResponse(buff, sizeof(buff), 2);

    if( strstr(buff, "STA") || strstr(buff,"AP") )
    {
        // Parsing Gateway ( IF/SSID/IP-Addr/Gateway/MAC/TxPower(dBm)/RSSI(-dBm) )
        token = strtok(buff, "/");  token = strtok(NULL, "/");
        token = strtok(NULL, "/");  token = strtok(NULL, "/");
        token = strtok(NULL, "/");  token = strtok(NULL, "/");
        //token = strtok(NULL, "/");

        ret = -atoi(token);
    }

    return ret;
}

uint8_t WizFi310Drv::getConnectionStatus()
{
    char buff[CMD_BUFFER_SIZE];

    if( sendCmd(F("AT+WSTATUS\r")) == -1 )
        return WL_NO_SHIELD;

    getResponse(buff, sizeof(buff), 2);
    LOGDEBUG(buff);

    if( strstr(buff, "Down") )
        return WL_DISCONNECTED;
    if( strstr(buff, "STA") || strstr(buff, "AP") )
        return WL_CONNECTED;

    return WL_IDLE_STATUS;
}

bool WizFi310Drv::getClientState(uint8_t sock)
{
    if( _state[sock] == NA_STATE )
    {
        return false;
    }

    return true;
}

char* WizFi310Drv::getFwVersion()
{
    sendCmd(F("AT+MINFO\r"));
    getResponse(fwVersion, sizeof(fwVersion), 2);

    return fwVersion;
}

// Start server TCP on port specified
bool WizFi310Drv::startServer(uint16_t port)
{
}

bool WizFi310Drv::startUdpServer(uint8_t sock, uint16_t port)
{
}


bool WizFi310Drv::startClient(const char* host, uint16_t port, uint8_t sock, uint8_t protMode)
{
    LOGDEBUG2(F("> startClient"), host, port);
    int status=0;
    bool is_ip,ret=false;
    char host_ip[16]={0,};
    char resp_ok[7] = "";
    char resp_con[15] = "";
    IPAddress ip;

    is_ip = WXParse_Ip((uint8_t*)host,ip);
    if( is_ip == true )
    {
        sprintf_P(host_ip,PSTR("%d.%d.%d.%d"),ip[0],ip[1],ip[2],ip[3]);
    }
    else
    {
        if( sendCmd(F("AT+FDNS=%s,%d\r"),10000,host,10000) == -1)
        {
			return false;
        }

        getResponse(host_ip, sizeof(host_ip), 1);
    }

    sprintf_P(resp_ok,PSTR("[OK]\r\n"));
    sprintf_P(resp_con,PSTR("[CONNECT %d]\r\n"), sock);

    if(protMode == TCP_MODE)
    {
//        sendCmd(F("AT+SMGMT=ALL\r"));
        status = SendCmdWithTag(F("AT+SCON=O,TCN,%s,%d,,0\r"),resp_ok,resp_con,10000,host_ip,port);
        ringBuf.reset();
    }
    else if(protMode == SSL_MODE)
    {
//        sendCmd(F("AT+SMGMT=ALL\r"));
        status = SendCmdWithTag(F("AT+SCON=O,TCS,%s,%d,,0\r"),resp_ok,resp_con,10000,host_ip,port);
        ringBuf.reset();
    }
    else if(protMode == UDP_MODE)
    {
//        sendCmd(F("AT+SMGMT=ALL\r"));
        status = SendCmdWithTag(F("AT+SCON=O,UCN,%s,%d,%d,0\r"),resp_ok,resp_con,10000,host_ip,port,_localPort);
        ringBuf.reset();
    }

    if( status == 0 )
    {
		_state[sock] = sock;
        ret = true;
    }
    else
    {
		_state[sock] = NA_STATE;
        ret = false;
    }

    return ret;
}   
    
void WizFi310Drv::stopClient(uint8_t sock)
{
    char cmdBuf[50];

	if( _state[sock] == sock && ringBuf.available() )
        return;


	sprintf_P(cmdBuf, PSTR("AT+SMGMT=%d\r"),sock);
	WizFi310Serial->print(cmdBuf);
    if( readUntil(1000) == TAG_OK )
    {
		_state[sock] = NA_STATE;
		//m_is_server_run = false;
    }
}

uint8_t WizFi310Drv::getServerState(uint8_t sock)
{
}

////////////////////////////////////////////////////////////////////////////
// TCP/IP functions
////////////////////////////////////////////////////////////////////////////
uint16_t WizFi310Drv::availData(uint8_t connId)
{
	uint8_t recved_byte;

	if( WizFi310Serial->available() )
	{
		recved_byte = (uint8_t)WizFi310Serial->read();
		parsingData(recved_byte);
	}

	return ringBuf.available();
}

static uint8_t tempIP[18]={0,};
static uint8_t tempIP_idx=0;
static uint8_t tmp_evnt_idx=0;


void WizFi310Drv::parsingData(uint8_t recv_data)
{
	switch(m_esc_state)
    {
    case ESC_IDLE:
		m_recved_len = 0;
        tempIP_idx = 0;
        if(recv_data == '{')
        {
			m_esc_state = ESC_CID;
        }
        else if(recv_data == '[')
        {
			m_esc_state = ESC_EVENT;
        }
        break;
    case ESC_EVENT:
		if( recv_data == 'D')
		{
			m_esc_state = ESC_EVENT_DISCONNECT;
			tmp_evnt_idx = 2;
		}
		else if( recv_data == 'L')
		{
			m_esc_state = ESC_EVENT_LINKDOWN;
			tmp_evnt_idx = 2;
		}
		else
		{
			m_esc_state = ESC_IDLE;
		}
		break;
    case ESC_CID:
        if( (recv_data >= '0') && (recv_data <= '9') )
        {

        }
        else if( recv_data == ',')
        {
			m_esc_state = ESC_PEERIP;
        }
        else
        {
			m_esc_state = ESC_IDLE;
        }
        break;
    case ESC_PEERIP:
        if( ((recv_data >= '0') && (recv_data <= '9')) || (recv_data == '.'))
        {
            tempIP[tempIP_idx++] = recv_data;
        }
        else if(recv_data == ',')
        {
            char *token;
            token = strtok((char*)tempIP,".");  _remoteIP[0] = atoi(token);
            token = strtok(NULL,".");   _remoteIP[1] = atoi(token);
            token = strtok(NULL,".");   _remoteIP[2] = atoi(token);
            token = strtok(NULL,".");   _remoteIP[3] = atoi(token);
            m_esc_state = ESC_PEERPORT;
            tempIP_idx = 0;
            _remotePort = 0;
        }
        else
        {
			m_esc_state = ESC_IDLE;
        }
        break;
    case ESC_PEERPORT:
        if((recv_data >= '0') && (recv_data <= '9'))
        {
            _remotePort *= 10;
            _remotePort += (uint16_t)(recv_data - '0');
        }
        else if(recv_data == ',')
        {
			m_esc_state = ESC_LENGTH;
        }
        else m_esc_state = ESC_IDLE;
        break;
    case ESC_LENGTH:
        if((recv_data >= '0') && (recv_data <= '9'))
        {
			m_recved_len *= 10;
			m_recved_len += (uint16_t)(recv_data - '0');
        }
        else if(recv_data == '}')
        {
			m_esc_state = ESC_RECV_DATA;
        }
        else m_esc_state = ESC_IDLE;
        break;
    case ESC_RECV_DATA:
//        if( ringBuf.available() >= (CMD_BUFFER_SIZE - 50) )
//        {
//			LOGDEBUG2("ringBuf threshold is over", (char)recv_data, ringBuf.available() );
//			m_recved_len--;
//			break;
//        }

        ringBuf.write(recv_data);
        m_recved_len--;

        if(m_recved_len <= 0)
        {
			m_recved_len = 0;
            m_esc_state = ESC_IDLE;
        }
        break;
    case ESC_EVENT_DISCONNECT:
		if( tmp_evnt_idx == 12 && ( recv_data >= '0' && recv_data <= '8') )
		{
			_state[recv_data-'0'] = NA_STATE;
			tmp_evnt_idx++;
		}
		else if( tmp_evnt_idx >= 13 && tmp_evnt_idx <= 15 )
		{
			tmp_evnt_idx++;
		}
		else if( (tmp_evnt_idx <= 11) && recv_data == WIZFI310EVNT[0][tmp_evnt_idx] )
		{
			tmp_evnt_idx++;
		}
		else
		{
			m_esc_state = ESC_IDLE;
			tmp_evnt_idx = 0;
		}
		break;
    case ESC_EVENT_LINKDOWN:
		if( tmp_evnt_idx > 18)
		{
			m_esc_state = ESC_IDLE;
			tmp_evnt_idx = 0;
			break;
		}

		if ( recv_data == WIZFI310EVNT[1][tmp_evnt_idx])
		{
			tmp_evnt_idx++;
			if( tmp_evnt_idx > 18 )
			{
				m_esc_state = ESC_IDLE;
				tmp_evnt_idx = 0;
			}
		}
		else
		{
			m_esc_state = ESC_IDLE;
			tmp_evnt_idx = 0;
		}
		break;
    }
}

bool WizFi310Drv::getData(uint8_t connId, uint8_t *data, bool peek, bool* connClose)
{
	int ch;

	long _startMillis = millis();
    do
    {
        if( ringBuf.available() )
        {
			ch = ringBuf.read();
			if( ch == -1)	return false;

			*data = (uint8_t)ch;
            return true;
        }
    }while((millis() - _startMillis) < 2000);

    // timed out, reset the buffer
    LOGERROR(F("TIMEOUT"));
    *data = 0;

    return false;
}

int WizFi310Drv::getDataBuf(uint8_t connId, uint8_t *buf, uint16_t bufSize)
{
    uint16_t i,idx=0;
    int recv_data=0;

    for(i=0; i<bufSize; i++)
    {
		recv_data = ringBuf.read();
		if( recv_data > 0 )
		{
			buf[idx] = (uint8_t)recv_data;
			idx++;
		}
		else
		{
			break;
		}

		//availData(connId);
    }

//    Serial.println("=====");
//    Serial.println(idx);
//    Serial.println("=====");
    buf[idx] = '\0';
    return idx;
}

bool WizFi310Drv::sendData(uint8_t sock, const uint8_t *data, uint16_t len)
{
    LOGDEBUG(F(">Send Data"));
    int i, ret;
    uint8_t recved_byte=0;

    if( len > CMD_BUFFER_SIZE )
    {
		LOGDEBUG(F("DBG>>>>Error : send-data-size if too big"));
        return false;
    }

    ringBuf.reset();
    char cmdBuf[100]={0};
    sprintf_P(cmdBuf, PSTR("AT+SSEND=%d,,,%d\r"), sock, len);
    WizFi310Serial->print(cmdBuf);

    sprintf_P(cmdBuf, PSTR("[%d,,,%d]\r\n"), sock, len);
    ret = readUntil(1000,cmdBuf);
    if(ret != 0)	return false;

    for(i=0; i<(int)len; i++)
    {
		WizFi310Serial->write(data[i]);
    }

    ret = readUntil(1000);
    ringBuf.reset();
    if(ret != 0)	return false;

    return true;
}

bool WizFi310Drv::sendData(uint8_t sock, const __FlashStringHelper *data, uint16_t len, bool appendCrLf)
{
}

void WizFi310Drv::getRemoteIpAddress(IPAddress& ip)
{
}

uint16_t WizFi310Drv::getRemotePort()
{
}



////////////////////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////////////////////

int WizFi310Drv::getResponse(char* outStr, int outStrLen, int lineNum)
{
    int i,ret=0;

    for(i=0;i<lineNum;i++)
    {
        memset(outStr,0,outStrLen);
        ret = ringBuf.getLine(outStr,outStrLen,'\r',1);
    }

    return ret;
}

int WizFi310Drv::sendCmd(const __FlashStringHelper* cmd, int timeout, ...)
{
	char cmdBuf[CMD_BUFFER_SIZE];

	va_list args;
	va_start (args, timeout);

#ifdef __AVR__
    vsnprintf_P (cmdBuf, CMD_BUFFER_SIZE, (char*)cmd, args);
#else
    vsnprintf (cmdBuf, CMD_BUFFER_SIZE, (char*)cmd, args);
#endif
    va_end(args);

    wizfiEmptyBuf();
    ringBuf.reset();

    LOGDEBUG(F("----------------------------------------------"));
    LOGDEBUG1(F(">>"), cmdBuf);

    WizFi310Serial->print(cmdBuf);
    int idx = readUntil(timeout);

    LOGDEBUG1(F("---------------------------------------------- >"), idx);
    LOGDEBUG();

    return idx;
}

int WizFi310Drv::SendCmdWithTag(const __FlashStringHelper* cmd, const char* tag, const char* tag2, int timeout, ...)
{
    char cmdBuf[CMD_BUFFER_SIZE];

    va_list args;
    va_start (args, timeout);

#ifdef __AVR__
    vsnprintf_P (cmdBuf, CMD_BUFFER_SIZE, (char*)cmd, args);
#else
    vsnprintf (cmdBuf, CMD_BUFFER_SIZE, (char*)cmd, args);
#endif

    va_end (args);

    wizfiEmptyBuf(true);
    ringBuf.reset();

    LOGDEBUG(F("----------------------------------------------"));
    LOGDEBUG1(F(">>"), cmdBuf);

    WizFi310Serial->print(cmdBuf);
    int idx = readUntil(timeout, tag, tag2);

    LOGDEBUG1(F("---------------------------------------------- >"), idx);
    LOGDEBUG();

    return idx;
}

int WizFi310Drv::readUntil(int timeout, const char* tag, const char* tag2, const char* error)
{
	int ret = TAG_ERROR;
	unsigned long start = millis();
    uint8_t  recved_byte, is_found1=0, is_found2=0;

	while( (millis() - start < (unsigned long)timeout) and ret < 0 )
	{
		if( WizFi310Serial->available() )
		{
			recved_byte = (char)WizFi310Serial->read();
			ringBuf.push(recved_byte);
			LOGDEBUG0((char)recved_byte);
		}
		else
		{
            if( is_found1 == 0 && ringBuf.FindStr(tag))
            {
                is_found1 = 1;
            }
            if( is_found2 == 0 && ringBuf.FindStr(tag2))
            {
                is_found2 = 1;
            }
            if( ringBuf.FindStr(error) )
            {
				return TAG_ERROR;
            }
		}

        if ( is_found1 && is_found2 )
        {
            ret = TAG_OK;
            break;
        }
    }
    if (millis() - start >= (unsigned long)timeout)
    {
        LOGWARN(F(">>> TIMEOUT >>>"));
    }

    return ret;
}

void WizFi310Drv::wizfiEmptyBuf(bool warn)
{
    char c;
    int i=0;
    while(WizFi310Serial->available() > 0)
    {
        c = WizFi310Serial->read();
        if (i>0 and warn == true)
        {
            LOGDEBUG0(c);
        }
        i++;
    }
    if (i>0 and warn == true)
    {
        LOGDEBUG(F(""));
        LOGDEBUG1(F("Dirty characters in the serial buffer! >"), i);
    }
}

int WizFi310Drv::timedRead()
{
}

WizFi310Drv wizfi310Drv;
