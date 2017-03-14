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

#ifndef WizFi310Drv_h 
#define WizFi310Drv_h

#include "Stream.h"
#include "IPAddress.h"
//#include "WizFi310RingBuffer.h"
#include "WizFiRingBuffer.h"
#include "WizFi310_definitions.h"
#include "debug.h"

typedef enum eProtMode {TCP_MODE, UDP_MODE, SSL_MODE} tProtMode;
typedef enum ESC_State {ESC_IDLE, ESC_CID, ESC_PEERIP, ESC_PEERPORT, ESC_LENGTH,
                        ESC_RECV_DATA, ESC_EVENT, ESC_EVENT_DISCONNECT, ESC_EVENT_LINKDOWN, ESC_EVENT_CONNECT_CLIENT} tESC_State;

typedef enum {
        WL_FAILURE = -1,
        WL_SUCCESS = 1,
} wl_error_code_t;

/* Authentication modes */
enum wl_auth_mode {
    AUTH_MODE_INVALID,
    AUTH_MODE_AUTO,
    AUTH_MODE_OPEN_SYSTEM,
    AUTH_MODE_SHARED_KEY,
    AUTH_MODE_WPA,
    AUTH_MODE_WPA2,
    AUTH_MODE_WPA_PSK,
    AUTH_MODE_WPA2_PSK
};


typedef enum {
    WL_NO_SHIELD = 255,
    WL_IDLE_STATUS = 0,
    //WL_NO_SSID_AVAIL,
    //WL_SCAN_COMPLETED,
    WL_CONNECTED,
    WL_CONNECT_FAILED,
    //WL_CONNECTION_LOST,
    WL_DISCONNECTED
} wl_status_t;

/* Encryption modes */
enum wl_wizfi_type {
    WIZ_TYPE_NONE = 0,
    WIZ_TYPE_WEP = 1,
    WIZ_TYPE_WPA = 2,
    WIZ_TYPE_WPA_AES = 3,
    WIZ_TYPE_WPA2_AES = 4,
    WIZ_TYPE_WPA2_TKIP = 5,
    WIZ_TYPE_WPA2_MIXED = 6
};


enum wl_tcp_state {
    CLOSED      = 0,
    LISTEN      = 1,
    SYN_SENT    = 2,
    SYN_RCVD    = 3,
    ESTABLISHED = 4,
    FIN_WAIT_1  = 5,
    FIN_WAIT_2  = 6,
    CLOSE_WAIT  = 7,
    CLOSING     = 8,
    LAST_ACK    = 9,
    TIME_WAIT   = 10
};



class WizFi310Drv
{
public:
    WizFi310Drv ();
    static int16_t _state[MAX_SOCK_NUM];

private:
    static void   wifiDriverInit   (Stream *wizfiSerial);
    static void   reset            ();
    static bool   wifiConnect      (const char *ssid, const char *passphrase);
    static bool   wifiStartAP      (const char *ssid, const char *pwd, uint8_t channel, uint8_t encry);
    static int8_t disconnect       ();

    static void   config   (IPAddress ip, IPAddress subnet, IPAddress gw);
    static void   config   ();

    static uint8_t* getMacAddress ();
    static void     getIpAddress  (IPAddress &ip);
    static void     getNetmask    (IPAddress& mask);
    static void     getGateway    (IPAddress& gw);

    static char*    getCurrentSSID  ();
    static uint8_t* getCurrentBSSID ();
    static int32_t  getCurrentRSSI  ();

    static uint8_t getConnectionStatus ();
    static bool    getClientState      (uint8_t sock);
    static char*   getFwVersion        ();

    static uint8_t startServer    (uint16_t port);
    static bool startUdpServer (uint8_t sock, uint16_t port);
    static bool startClient (const char* host, uint16_t port, uint8_t sock, uint8_t protMode);
    static void stopClient  (uint8_t sock);

    static uint8_t getServerState (uint8_t sock);
    
    ////////////////////////////////////////////////////////////////////////////
    // TCP/IP functions
    ////////////////////////////////////////////////////////////////////////////
    static uint16_t availData  ();
    static void     parsingData(uint8_t recv_data);
    static bool     getData    (uint8_t connId, uint8_t *data, bool peek, bool* connClose);
    static int      getDataBuf (uint8_t connId, uint8_t *buf, uint16_t bufSize);
    static bool     sendData   (uint8_t sock, const uint8_t *data, uint16_t len);
    static bool     sendData   (uint8_t sock, const __FlashStringHelper *data, uint16_t len, bool appendCrLf=false);
    
    static void     getRemoteIpAddress (IPAddress& ip);
    static uint16_t getRemotePort();
    static uint8_t  getFirstSocket();

    static Stream *WizFi310Serial;

    static long _bufPos;
    static uint8_t _connId;

    static uint16_t _remotePort;

    // firmware version string
    static char     fwVersion[WL_FW_VER_LENGTH];
    static char     _ssid[WL_SSID_MAX_LENGTH];
    static uint8_t  _bssid[WL_MAC_ADDR_LENGTH];
    static uint8_t  _mac[WL_MAC_ADDR_LENGTH];
    static uint8_t  _localIp[WL_IPV4_LENGTH];
    static uint8_t  _remoteIP[WL_IPV4_LENGTH];

    //static WizFi310RingBuffer ringBuf;
    static WizFiRingBuffer ringBuf;

public:
    static int  getResponse     (char* outStr, int outStrLen, int lineNum);
    static int  sendCmd         (const __FlashStringHelper* cmd, int timeout=1000, ...);
    static int  SendCmdWithTag  (const __FlashStringHelper* cmd, const char* tag="[OK]", const char* tag2="", int timeout=10000, ...);
    static int  SendCmdWithTag  (const char* cmd, const char* tag, const char* tag2, int timeout=10000);
    static bool sendCmdGet      (const char* cmd, const char* startTag, const char* endTag, char* outStr, int outStrLen, int opt);
    static bool sendCmdGet      (const __FlashStringHelper* cmd, const __FlashStringHelper* startTag, const __FlashStringHelper* endTag, char* outStr, int outStrLen, int opt=0);
    static bool sendCmdGet      (const char* cmd, const __FlashStringHelper* startTag, const __FlashStringHelper* endTag, char* outStr, int outStrLen, int opt=0);

    static int  readUntil     (int timeout, const char* tag="[OK]\r\n", const char* tag2="", const char* error="[ERROR]\r\n");
    static void wizfiEmptyBuf (bool warn=true);

private:
    static bool m_use_dhcp;

    static uint16_t m_esc_state;
    static int      m_recved_len;
    static uint8_t  m_client_sock;

    static uint16_t _localPort;
    
    friend class WizFi310Class;
    friend class WiFiClient;
    friend class WiFiServer;
    friend class WiFiUDP;
};

extern WizFi310Drv wizfi310Drv;

#endif 
