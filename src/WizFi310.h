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

#ifndef WizFi310_h
#define WizFi310_h

#include <Arduino.h>
#include <Stream.h>
#include <IPAddress.h>
#include <inttypes.h>

#include "WizFi310Client.h"
#include "WizFi310Server.h"
#include "utility/WizFiRingBuffer.h"
#include "utility/WizFi310Drv.h"
#include "utility/debug.h"

class WizFi310Class
{
public:
    WizFi310Class();


    static void init(Stream *espSerial);
    static char* firmwareVersion();
    
    static int   begin    (const char *ssid, const char *passphrase);
    static int   begin    (const char *ssid);
    static int   beginAP  (char *ssid, uint8_t channel, const char *pwd, uint8_t encry);
    static int   beginAP  (char *ssid);
    static int   beginAP  (char *ssid, uint8_t channel);
    
    static void  config   (IPAddress ip, IPAddress subnet, IPAddress gw);
    static void  config   ();
    static void  configAP (IPAddress ip);
    static int   disconnect ();

    static String   macAddress (void);
    static uint8_t* macAddress (uint8_t *mac);

    static IPAddress localIP    ();
    static IPAddress subnetMask ();
    static IPAddress gatewayIP  ();

    
    static char*    SSID  ();
    static uint8_t* BSSID (uint8_t* bssid);
    static int32_t  RSSI  ();
    static uint8_t  status();


private:
    static uint8_t wizfiMode;
};

extern WizFi310Class WiFi;

#endif
