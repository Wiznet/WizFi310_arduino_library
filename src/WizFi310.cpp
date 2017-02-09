#include "WizFi310.h"

WizFi310Class::WizFi310Class()
{
}

void WizFi310Class::init(Stream *wizfiSerial)
{
    LOGINFO(F("Initializing WizFi310 module"));
    WizFi310Drv::wifiDriverInit(wizfiSerial);
}

char* WizFi310Class::firmwareVersion()
{
    return WizFi310Drv::getFwVersion();
}

int WizFi310Class::begin(const char *ssid, const char *passphrase)
{
    if ( WizFi310Drv::wifiConnect(ssid, passphrase) )
        return WL_CONNECTED;

    return WL_CONNECT_FAILED;
}

int WizFi310Class::begin(const char *ssid)
{
    if ( WizFi310Drv::wifiConnect(ssid, "") )
        return WL_CONNECTED;

    return WL_CONNECT_FAILED;
}

int WizFi310Class::beginAP(char *ssid, uint8_t channel, const char *pwd, uint8_t encry)
{
    if( WizFi310Drv::wifiStartAP(ssid, pwd, channel, encry) )
        return WL_CONNECTED;

    return WL_CONNECT_FAILED;
}

int WizFi310Class::beginAP(char *ssid)
{
    return beginAP(ssid, 10, "", WIZ_TYPE_NONE);
}

int WizFi310Class::beginAP(char *ssid, uint8_t channel)
{
    return beginAP(ssid, channel, "", WIZ_TYPE_NONE);
}

void WizFi310Class::config(IPAddress ip, IPAddress subnet, IPAddress gw) 
{
    WizFi310Drv::config(ip, subnet, gw);
}

void WizFi310Class::configAP(IPAddress ip)
{
    IPAddress subnet(255,255,255,0);
    IPAddress gw = ip;

    WizFi310Drv::config(ip, subnet, gw);
}

void WizFi310Class::config(void)
{
    WizFi310Drv::config();
}

int WizFi310Class::disconnect()
{
    return WizFi310Drv::disconnect();
}

String WizFi310Class::macAddress(void)
{
    uint8_t mac[6];
    char macStr[18] = {0};

    macAddress(mac);

    sprintf_P(macStr, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"),mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return String(macStr);
}

uint8_t* WizFi310Class::macAddress(uint8_t *mac)
{
    uint8_t* _mac = WizFi310Drv::getMacAddress();
    memcpy(mac, _mac, WL_MAC_ADDR_LENGTH);
    return mac;
}

IPAddress WizFi310Class::localIP()
{
    IPAddress ret;
    WizFi310Drv::getIpAddress(ret);
    return ret;
}

IPAddress WizFi310Class::subnetMask()
{
    IPAddress mask;
    WizFi310Drv::getNetmask(mask);
    return mask;
}

IPAddress WizFi310Class::gatewayIP()
{
    IPAddress gw;
    WizFi310Drv::getGateway(gw);
    return gw;
}

char* WizFi310Class::SSID()
{
    return WizFi310Drv::getCurrentSSID();
}

uint8_t* WizFi310Class::BSSID(uint8_t* bssid)
{
    return WizFi310Drv::getCurrentBSSID();
}

int32_t WizFi310Class::RSSI()
{
    return WizFi310Drv::getCurrentRSSI();
}

uint8_t WizFi310Class::status()
{
    return WizFi310Drv::getConnectionStatus(); 
}

WizFi310Class WiFi;
