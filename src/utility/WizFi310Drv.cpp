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
#define CMD_BUFFER_SIZE 200

const char* WIZFI310TAGS[] =
{
    "[OK]\r\n",
    "[ERROR]\r\n"
//  "[CONNECT ",
//    "[DISCONNECT ",
//    "[Link-Up Event]\r\n",
//    "[Link-Down Event]\r\n"
};

typedef enum
{
    TAG_OK,
    TAG_ERROR,
//  TAG_CONNECT,
//  TAG_DISCONNECT,
//    TAG_LINKUP,
//    TAG_LINKDOWN
    TAG_NUM
} TagsEnum;

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
    
    reset();
    
    for(int i=0; i<5; i++)
    {
        if (sendCmd(F("AT\r")) == TAG_OK)
        {
            initOK = true;
            break;
        }
        delay(1000);
    }
//    sendCmd(F("AT+MRESET\r"));
//    for(int i=0; i<5; i++)
//    {
//        if (sendCmd(F("AT\r")) == TAG_OK)
//        {
//            initOK = true;
//            break;
//        }
//        delay(1000);
//    }

    if (!initOK)
    {
        LOGERROR(F("Cannot initialize WizFi310 module"));
        delay(5000);
        return;
    }

    sendCmd(F("AT+WLEAVE\r"));  
    sendCmd(F("AT+MECHO=0\r"));
    // check firmware version
    //getFwVersion();

}

void WizFi310Drv::reset()
{

}

bool WizFi310Drv::wifiConnect(const char*ssid, const char *passphrase)
{
    LOGDEBUG(F("> wifiConnect"));

    if( sendCmd(F("AT+WLEAVE\r") ) != TAG_OK )                      return false;
    if( sendCmd(F("AT+WSET=0,%s\r"), 1000, ssid) != TAG_OK )        return false;
    if( sendCmd(F("AT+WSEC=0,,%s\r"), 1000, passphrase) != TAG_OK ) return false;
    
    if( m_use_dhcp )
    {
        if( sendCmd(F("AT+WNET=1\r")) != TAG_OK ) return false;
    }

    //if( sendCmd(F("AT+WJOIN\r"), 60000, "") != TAG_LINKUP ) return false;
    if( sendCmd(F("AT+WJOIN\r"), 60000, "") != TAG_OK ) return false;
    
    LOGINFO1(F("Connected to"), ssid);
    return true;
}

bool WizFi310Drv::wifiStartAP(const char *ssid, const char *pwd, uint8_t channel, uint8_t encry)
{
    LOGDEBUG(F("> wifiStartAP"));
    char ch_enc[5]={0,};
    if      ( encry == WIZ_TYPE_NONE )      strcpy(ch_enc,"OPEN");
    else if ( encry == WIZ_TYPE_WEP  )      strcpy(ch_enc,"WEP");
    else if ( encry == WIZ_TYPE_WPA  )      strcpy(ch_enc,"WPA");
    else if ( encry == WIZ_TYPE_WPA_AES )   strcpy(ch_enc,"WPAAES");
    else if ( encry == WIZ_TYPE_WPA2_AES )  strcpy(ch_enc,"WPA2AES");
    else if ( encry == WIZ_TYPE_WPA2_TKIP ) strcpy(ch_enc,"WPA2TKIP");
    else if ( encry == WIZ_TYPE_WPA2_MIXED) strcpy(ch_enc,"WPA2");

    if( sendCmd(F("AT+WLEAVE\r") ) != TAG_OK )                          return false;
    if( sendCmd(F("AT+WSET=1,%s\r"),1000, ssid) != TAG_OK )             return false;
    if( sendCmd(F("AT+WSEC=1,%s,%s\r"),1000, ch_enc,pwd) != TAG_OK )    return false;


    //if( sendCmd(F("AT+WJOIN\r"), 60000, "" )  != TAG_LINKUP )   return false;
    if( sendCmd(F("AT+WJOIN\r"), 60000, "" )  != TAG_OK )   return false;

    LOGINFO1(F("Access point started"), ssid);
    return true;
}

int8_t WizFi310Drv::disconnect()
{
    LOGDEBUG(F("> disconnect"));

    if(sendCmd(F("AT+WLEAVE\r")) == TAG_OK)
        return WL_DISCONNECTED;
    delay(2000);
    wizfiEmptyBuf(false);

    return WL_DISCONNECTED;
}

void WizFi310Drv::config(IPAddress ip, IPAddress subnet, IPAddress gw)
{
    LOGDEBUG(F("> config(IP,Subnet,Gateway)"));
    char buf_ip[16];
    char buf_sub[16];
    char buf_gw[16];

    sprintf_P(buf_ip, PSTR("%d.%d.%d.%d"),ip[0], ip[1], ip[2], ip[3] );
    sprintf_P(buf_sub, PSTR("%d.%d.%d.%d"), subnet[0], subnet[1], subnet[2], subnet[3] );
    sprintf_P(buf_gw, PSTR("%d.%d.%d.%d"), gw[0], gw[1], gw[2], gw[3] );
    
    if( sendCmd(F("AT+WNET=0,%s,%s,%s\r"), 1000, buf_ip, buf_sub, buf_gw) == TAG_OK)
    {
        m_use_dhcp = false;
    }
}

void WizFi310Drv::config(void)
{
    LOGDEBUG(F("> config(DHCP)"));
    
    sendCmd(F("AT+WNET=1\r"));
}

uint8_t* WizFi310Drv::getMacAddress()
{
    LOGDEBUG(F("> getMacAddress"));

    memset(_mac, 0, WL_MAC_ADDR_LENGTH);
    char buf[20];
    if (sendCmdGet(F("AT+MMAC=?\r"),F(""),F("\r\n"),buf, sizeof(buf)))
    {
        char* token;

        token = strtok(buf, ":");
        _mac[0] = (byte)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        _mac[1] = (byte)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        _mac[2] = (byte)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        _mac[3] = (byte)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        _mac[4] = (byte)strtol(token, NULL, 16);
        token = strtok(NULL, ":");
        _mac[5] = (byte)strtol(token, NULL, 16);
    }

    return _mac;
}

void WizFi310Drv::getIpAddress(IPAddress &ip)
{
    char buff[CMD_BUFFER_SIZE];
    char *token;
    char *p_ip;
    if ( sendCmdGet(F("AT+WSTATUS\r"),F("RSSI(-dBm)\r\n"),F("\r\n"),buff, sizeof(buff)) )
    {
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
            if( sendCmdGet(F("AT+WNET=?\r"),F(""),F("\r\n"),buff, sizeof(buff)) )
            {
                token = strtok(buff,",");   token = strtok(NULL,",");

                p_ip = strtok(token,".");   _localIp[0] = atoi(p_ip);
                p_ip = strtok(NULL,".");    _localIp[1] = atoi(p_ip);
                p_ip = strtok(NULL,".");    _localIp[2] = atoi(p_ip);
                p_ip = strtok(NULL,".");    _localIp[3] = atoi(p_ip);
            }
        }
    }

    ip = _localIp;
}

void WizFi310Drv::getNetmask(IPAddress& mask)
{
    char buff[CMD_BUFFER_SIZE];
    char *token;
    char *p_ip;

    if( sendCmdGet(F("AT+WNET=?\r"),F(""),F("\r\n"),buff,sizeof(buff)) )
    {
        token = strtok(buff,",");   token = strtok(NULL,",");

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

    if ( sendCmdGet(F("AT+WSTATUS\r"),F("RSSI(-dBm)\r\n"),F("\r\n"),buff, sizeof(buff)) )
    {
        if( strstr(buff, "STA") || strstr(buff, "AP") )
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
            if ( sendCmdGet(F("AT+WNET=?\r"),F(""),F("\r\n"),buff, sizeof(buff)) )
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
}

char* WizFi310Drv::getCurrentSSID()
{
    LOGDEBUG(F("> getCurrentSSID"));
    char buff[CMD_BUFFER_SIZE];
    char *token;

    memset(_ssid, 0, WL_SSID_MAX_LENGTH);

    if( sendCmdGet(F("AT+WSTATUS\r"),F("RSSI(-dBm)\r\n"),F("\r\n"),buff,sizeof(buff)) )
    {
        token = strtok(buff, "/");    token = strtok(NULL,",");
        memcpy(_ssid, token, WL_SSID_MAX_LENGTH);
    }

    return _ssid;
}

uint8_t* WizFi310Drv::getCurrentBSSID()
{

}

int32_t WizFi310Drv::getCurrentRSSI()
{
    LOGDEBUG(F("> getCurrentRSSI"));
    char buff[CMD_BUFFER_SIZE];
    char *token;
    int ret=0;

    if ( sendCmdGet(F("AT+WSTATUS\r"),F("RSSI(-dBm)\r\n"),F("\r\n"),buff, sizeof(buff)) )
    {
        if( strstr(buff, "STA") || strstr(buff,"AP") )
        {
            // Parsing Gateway ( IF/SSID/IP-Addr/Gateway/MAC/TxPower(dBm)/RSSI(-dBm) )
            token = strtok(buff, "/");          token = strtok(NULL, "/");
            token = strtok(NULL, "/");          token = strtok(NULL, "/");
            token = strtok(NULL, "/");          token = strtok(NULL, "/");

            ret = -atoi(token);
        }
    }

    return ret;
}

uint8_t WizFi310Drv::getConnectionStatus()
{
    char buff[CMD_BUFFER_SIZE];

    if( sendCmdGet(F("AT+WSTATUS\r"),F("RSSI(-dBm)\r\n"),F("\r\n"),buff, sizeof(buff)) )
    {
        if( strstr(buff, "Down") )
            return WL_DISCONNECTED;
        if( strstr(buff, "STA") || strstr(buff, "AP") )
            return WL_CONNECTED;
    }   
    return WL_IDLE_STATUS;
}

uint8_t WizFi310Drv::getClientState(uint8_t sock)
{
    if( _state[sock] != NA_STATE )
       return true;

    return false;   
}

char* WizFi310Drv::getFwVersion()
{
    LOGDEBUG(F("> getFwVersion"));

    fwVersion[0] = 0;
    sendCmdGet(F("AT+MINFO\r"), F("FW version/HW version\r\n"), F("\r\n"), fwVersion, sizeof(fwVersion));

    return fwVersion;
}

// Start server TCP on port specified
bool WizFi310Drv::startServer(uint16_t port)
{
    int ret;
    LOGDEBUG1(F("> startServer"), port);

    for(int i=0; i<3; i++)
    {
        sendCmd(F("AT\r"));
        ret = sendCmd(F("AT+SCON=O,TSN,,,%d,0\r"), 1000, port);
        if( ret == TAG_OK )
            return true;
    }
    return false;
}

bool WizFi310Drv::startUdpServer(uint8_t sock, uint16_t port)
{
    bool ret=false;
    char cmdBuf[100];
    char buff[CMD_BUFFER_SIZE];

    LOGDEBUG1(F("> start Udp Server"), port);

    sendCmd(F("AT\r"));
    sprintf_P(cmdBuf, PSTR("AT+SCON=O,USN,,,%d,0\r"), port);
    ret = sendCmdGet(cmdBuf, F("[CONNECT "),F("]\r\n"), buff, sizeof(buff),2);
    if( (buff[0] - '0') != sock )
    {
        sendCmd(F("AT+SMGMT=ALL\r"));
        ret = false;
    }

    return ret;
}


bool WizFi310Drv::startClient(const char* host, uint16_t port, uint8_t sock, uint8_t protMode)
{
    LOGDEBUG2(F("> startClient"), host, port);
    
    bool is_ip,ret = false; 
    char host_ip[16]={0,};
    char cmdBuf[100];
    char buff[CMD_BUFFER_SIZE];
    IPAddress ip;

    is_ip = WXParse_Ip((uint8_t*)host,ip);
    if( is_ip == true )
    {
        sprintf_P(host_ip,PSTR("%d.%d.%d.%d"),ip[0],ip[1],ip[2],ip[3]);
    }
    else
    {
        sprintf_P(cmdBuf, PSTR("AT+FDNS=%s,%d\r"),host,3000);
        if(sendCmdGet(cmdBuf,F(""),F("\r\n"),buff,sizeof(buff),1))
        {
            strncpy(host_ip,buff,sizeof(host_ip));
        }
    }

    memset(buff,0,sizeof(buff));
    if(protMode == TCP_MODE)
    {
        sendCmd(F("AT+SMGMT=ALL\r"));
        sprintf_P(cmdBuf, PSTR("AT+SCON=O,TCN,%s,%d,,0\r"),host_ip,port);
    }
    else if(protMode == SSL_MODE)
    {
        sendCmd(F("AT+SMGMT=ALL\r"));
        sprintf_P(cmdBuf, PSTR("AT+SCON=O,TCS,%s,%d,,0\r"),host_ip,port);
    }
    else if(protMode == UDP_MODE)
    {
        sendCmd(F("AT+SMGMT=ALL\r"));
        sprintf_P(cmdBuf, PSTR("AT+SCON=O,UCN,%s,%d,%d,0\r"),host_ip,port,_localPort);
    }

    ret = sendCmdGet(cmdBuf,F("[CONNECT "),F("]\r\n"),buff,sizeof(buff),2);
    if( (buff[0] - '0') != sock )
    {
        sendCmd(F("AT+SMGMT=ALL\r"));
        ret = false;
    }
    else{
        LOGDEBUG2("Sock Num : ", buff[0], sock );
    }

    return ret;
}   
    
void WizFi310Drv::stopClient(uint8_t sock)
{
    LOGDEBUG1(F("> stopClient"), sock);

    sendCmd(F("AT+SMGMT=%d\r"), 1000, sock);
}

uint8_t WizFi310Drv::getServerState(uint8_t sock)
{
    return 0;
}
////////////////////////////////////////////////////////////////////////////
// TCP/IP functions
////////////////////////////////////////////////////////////////////////////
uint16_t WizFi310Drv::availData(uint8_t connId)
{   
    if (_bufPos > 0)
    {
        if (_connId==connId)
        {
            return _bufPos;
        }
        else if (_connId == 0)
        {
            return _bufPos;
        }
    }

    int bytes = WizFi310Serial->available();
    if (bytes)
    {
        if ( WizFi310Serial->peek() == '{' )
        {
            // Format : {cid,dest_ip,dest_port,length}
            _connId = WizFi310Serial->parseInt();          // cid
            WizFi310Serial->read();                        // ,
            _remoteIp[0] = WizFi310Serial->parseInt();     // remote IP
            WizFi310Serial->read();                        // .
            _remoteIp[1] = WizFi310Serial->parseInt();
            WizFi310Serial->read();                        // .
            _remoteIp[2] = WizFi310Serial->parseInt();
            WizFi310Serial->read();                        // .
            _remoteIp[3] = WizFi310Serial->parseInt();
            WizFi310Serial->read();                        // ,
            _remotePort = WizFi310Serial->parseInt();      // dest_port
            WizFi310Serial->read();                        // ,
            _bufPos = WizFi310Serial->parseInt();          // length
            WizFi310Serial->read();                        // }

            //LOGDEBUG();
            //LOGDEBUG2(F("Data packet"), _connId, _bufPos);

            if(_connId==connId || connId==0)
                return _bufPos;
        }
        else if( WizFi310Serial->peek() == '[' )
        {
            WizFi310Serial->read();
            if( WizFi310Serial->peek() == 'C' )
            {
                if( WizFi310Serial->find((char*)"CONNECT ") )
                {
                    _connId = WizFi310Serial->parseInt();
                    WizFi310Serial->read();                 // ]
                    WizFi310Serial->read();                 // 0x0d
                    WizFi310Serial->read();                 // 0x0a
                    WizFi310Drv::_state[_connId] = _connId;
                }
            }
            else if( WizFi310Serial->peek() == 'D' )
            {
                if( WizFi310Serial->find((char*)"DISCONNECT ") )
                {
                    _connId = WizFi310Serial->parseInt();
                    WizFi310Serial->read();                 // ]
                    WizFi310Serial->read();                 // 0x0d
                    WizFi310Serial->read();                 // 0x0a
                    WizFi310Drv::_state[_connId] = NA_STATE;                        
                }
            }
            else if( WizFi310Serial->peek() == 'L' )
            {
                if( WizFi310Serial->find((char*)"Link-Down Event]") )
                {
                    WizFi310Serial->read();                 // 0x0d
                    WizFi310Serial->read();                 // 0x0a
                    
                    for(int i=0; i<MAX_SOCK_NUM; i++)
                    {
                        WizFi310Drv::_state[i] = NA_STATE;
                    }                    
                    disconnect();
                }
            }
        }
    }

    return bytes;
}

bool WizFi310Drv::getData(uint8_t connId, uint8_t *data, bool peek, bool* connClose)
{
    if (connId!=_connId)
    {
        return false;
    }

    long _startMillis = millis();
    do
    {
        if (WizFi310Serial->available())
        {
            if (peek)
            {
                *data = (char)WizFi310Serial->peek();
                return true;
            }

            if(_bufPos <= 0)
            {
                uint8_t ch = WizFi310Serial->read();
                if(ch == '[')
                {
                    char msg[20];
                    sprintf_P(msg, PSTR("DISCONNECT %d]"), connId);
                    //WizFi310Serial->find((char*)"DISCONNECT ");
                    if( WizFi310Serial->find((char*)msg) )
                    {
                        //LOGDEBUG();
                        //LOGDEBUG(F("Connection closed"));
                        *connClose=true;
                    }
                }
                return false;
            }
            else
            {
                *data = (char)WizFi310Serial->read();
                _bufPos--;
                return true;
            }
        }
    } while(millis() - _startMillis < 2000);

    // timed out, reset the buffer
    LOGERROR1(F("TIMEOUT:"), _bufPos);

    _bufPos = 0;
    _connId = 0;
    *data = 0;
    
    return false;
}

int WizFi310Drv::getDataBuf(uint8_t connId, uint8_t *buf, uint16_t bufSize)
{
    if (connId!=_connId)
        return false;

    if(_bufPos<bufSize)
        bufSize = _bufPos;
    
    for(int i=0; i<bufSize; i++)
    {
        int c = timedRead();
        if(c==-1)
            return -1;
        
        buf[i] = (char)c;
        _bufPos--;
    }

    return bufSize;
}

bool WizFi310Drv::sendData(uint8_t sock, const uint8_t *data, uint16_t len)
{
    LOGDEBUG2(F("> sendData:"), sock, len);

    char cmdBuf[20];
    sprintf_P(cmdBuf, PSTR("AT+SSEND=%d,,,%d\r"), sock, len);
    WizFi310Serial->print(cmdBuf);

    WizFi310Serial->write(data, len);

    int idx = readUntil(2000);
    if(idx!=TAG_OK)
    {
        LOGERROR(F("Data packet send error (2)"));
        return false;
    }

    return true;
}

bool WizFi310Drv::sendData(uint8_t sock, const __FlashStringHelper *data, uint16_t len, bool appendCrLf)
{
    LOGDEBUG2(F("> sendData:"), sock, len);

    char cmdBuf[20];
    uint16_t len2 = len + 2*appendCrLf;

    sprintf_P(cmdBuf, PSTR("AT+SSEND=%d,,,%d\r"), sock, len2);
    WizFi310Serial->print(cmdBuf);

    //espSerial->write(data, len);
    PGM_P p = reinterpret_cast<PGM_P>(data);
    for (int i=0; i<len; i++)
    {
        unsigned char c = pgm_read_byte(p++);
        WizFi310Serial->write(c);
    }
    if (appendCrLf)
    {
        WizFi310Serial->write('\r');
        WizFi310Serial->write('\n');
    }

    int idx = readUntil(2000);
    if(idx!=TAG_OK)
    {
        LOGERROR(F("Data packet send error (2)"));
        return false;
    }

    return true;
}

void WizFi310Drv::getRemoteIpAddress(IPAddress& ip)
{
    ip = _remoteIp;
}

uint16_t WizFi310Drv::getRemotePort()
{
    return _remotePort;
}



////////////////////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////////////////////

int WizFi310Drv::sendCmd(const __FlashStringHelper* cmd, int timeout)
{
    wizfiEmptyBuf();

    LOGDEBUG(F("----------------------------------------------"));
    LOGDEBUG1(F(">>"), cmd);

    //WizFi310Serial->println(cmd);
    WizFi310Serial->print(cmd);

    int idx = readUntil(timeout);

    LOGDEBUG1(F("---------------------------------------------- >"), idx);
    LOGDEBUG();

    return idx; 
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
    va_end (args);

    wizfiEmptyBuf();
    LOGDEBUG(F("----------------------------------------------"));
    LOGDEBUG1(F(">>"), cmdBuf);

    //WizFi310Serial->println(cmdBuf);
    WizFi310Serial->print(cmdBuf);

    int idx = readUntil(timeout);

    LOGDEBUG1(F("---------------------------------------------- >"), idx);
    LOGDEBUG();

    return idx;
}

bool WizFi310Drv::sendCmdGet(const char* cmd, const char* startTag, const char* endTag, char* outStr, int outStrLen, int opt)
{
    int idx;
    bool ret = false;

    outStr[0] = 0;
    wizfiEmptyBuf();

    LOGDEBUG(F("----------------------------------------------"));
    LOGDEBUG1(F(">>"), cmd);

    // send AT command to WizFi310
    WizFi310Serial->print(cmd);

    if(opt == 1) // opt 1 is used when it doesn't need to find startTag.
    {
        idx = NUM_WIZFI_TAGS;
    }
    else if(opt == 2)
    {
        // read result until the startTag is found but except WIZFI310TAGS 
        idx = readUntil(1000, startTag,false);
    }
    else
    {
        // read result until the startTag is found buf if include WIZFI310TAGS, it will return
        idx = readUntil(1000, startTag);
    }

    if(idx == NUM_WIZFI_TAGS)
    {
        // clean the buffer to get a clean string
        ringBuf.init();

        if(opt == 2)
        {
            // start tag found, search the endTag
            idx = readUntil(3000, endTag,false);
        }
        else
        {
            // start tag found, search the endTag
            idx = readUntil(1000, endTag);
        }   

        if(idx==NUM_WIZFI_TAGS)
        {
            // end tag found
            // copy result to output buffer avoiding overflow
            ringBuf.getStrN(outStr, strlen(endTag), outStrLen-1);

            if( opt != 2)
            {
                // read the remaining part of the response
                readUntil(2000);
            }

            ret = true;
        }
        else
        {
            LOGWARN(F("End tag not found"));
        }
    }
    else if(idx>=0 and idx<NUM_WIZFI_TAGS)
    {
        // the command has returned but no start tag is found
        LOGDEBUG1(F("No start tag found:"), idx);
    }
    else
    {
        // the command has returned but no tag is found
        LOGWARN(F("No tag found"));
    }

    LOGDEBUG(F("---------------------------------------------- >\r\n"));
    LOGDEBUG(outStr);

    return ret;
}

bool WizFi310Drv::sendCmdGet(const __FlashStringHelper* cmd, const __FlashStringHelper* startTag, const __FlashStringHelper* endTag, char* outStr, int outStrLen, int opt)
{
    char _startTag[strlen_P((char*)startTag)+1];
    strcpy_P(_startTag,  (char*)startTag);

    char _endTag[strlen_P((char*)endTag)+1];
    strcpy_P(_endTag,  (char*)endTag);

    char _cmd[strlen_P((char*)cmd)+1];
    strcpy_P(_cmd, (char*)cmd);

    return sendCmdGet(_cmd, _startTag, _endTag, outStr, outStrLen, opt);
}

bool WizFi310Drv::sendCmdGet(const char* cmd, const __FlashStringHelper* startTag, const __FlashStringHelper* endTag, char* outStr, int outStrLen, int opt)
{
    char _startTag[strlen_P((char*)startTag)+1];
    strcpy_P(_startTag,  (char*)startTag);

    char _endTag[strlen_P((char*)endTag)+1];
    strcpy_P(_endTag,  (char*)endTag);

    return sendCmdGet(cmd, _startTag, _endTag, outStr, outStrLen, opt);
}

int WizFi310Drv::readUntil(int timeout, const char* tag, bool findTags)
{
    ringBuf.reset();

    char c;
    unsigned long start = millis();
    int ret = -1;

    while ((millis() - start < timeout) and ret < 0 )
    {
        if( WizFi310Serial->available() )
        {
            c = (char)WizFi310Serial->read();
            //LOGDEBUG0(c);
            ringBuf.push(c);

            if (tag!=NULL)
            {
                if (ringBuf.endsWith(tag))
                {
                    ret = NUM_WIZFI_TAGS;
                }
            }
            if(findTags)
            {
                for(int i=0; i<NUM_WIZFI_TAGS; i++)
                {
                    if (ringBuf.endsWith(WIZFI310TAGS[i]))
                    {
                        ret = i;
                        break;
                    }
                }
            }
        }
    }

    if (millis() - start >= timeout)
    {
        LOGWARN(F(">>> TIMEOUT >>>"));
    }

    LOGDEBUG0(ringBuf.getStr());
    

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
    int _timeout = 1000;
    int c;
    long _startMillis = millis();
    do
    {
        c = WizFi310Serial->read();
        if (c >= 0) return c;
    } while(millis() - _startMillis < _timeout);

    return -1;      // -1 indicates timeout
}

WizFi310Drv wizfi310Drv;
