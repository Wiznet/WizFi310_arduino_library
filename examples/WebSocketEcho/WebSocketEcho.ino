/**
 * @project WizFi310 WebSocket Echo
 * @brief   WebSocket Echo test using WebSocket-Fast library
 * @detail  refer to http://wiznetian.com/ & https://github.com/u0078867/Arduino-Websocket-Fast
 * @author  Kei ( http://openmaker.tistory.com/ )
 * @date    2017-03-02
 * @version 0.0.1
 * @Borad   WizArduino WiFi (Arduino Mega2560 + WizFi310)
 */

/* Includes ------------------------------------------------------------------*/
#include <WizFi310.h>
#include <WebSocketClient.h>

// Here we define a maximum framelength to 64 bytes. Default is 256.
#define MAX_FRAME_LENGTH 256
// Define how many callback functions you have. Default is 1.
#define CALLBACK_FUNCTIONS 1
#define SERIAL_DEBUG Serial
#define SERIAL_WIFI  Serial3
#define CAPTURE_PIN A1
#define SERVER_PORT 80

const char* server_dns = "echo.websocket.org";

char ssid[] = "YOURSSID";    // your network SSID (name)
char pass[] = "YOURSSIDPASSWORD";          // your network password
int status = WL_IDLE_STATUS;       // the Wifi radio's status

WiFiClient client;
WebSocketClient webSocketClient;

void setup()
{

    SERIAL_DEBUG.begin(115200);

    while (!SERIAL_DEBUG)
        ;

    initWizFi310();

    SERIAL_DEBUG.println("====== WizFi310 WebSocket-Fast(Echo) ======");

    connectToServer();

    handshakeWithServer();
}

void loop()
{
    String data;

    if (client.connected())
    {

        webSocketClient.getData(data);

        if (data.length() > 0)
        {
            SERIAL_DEBUG.print("Received data: ");
            SERIAL_DEBUG.println(data);
        }

        // capture the value of analog PIN, send it along
        pinMode(CAPTURE_PIN, INPUT);
        data = String(analogRead(CAPTURE_PIN));

        webSocketClient.sendData(data);

    }
    else
    {

        SERIAL_DEBUG.println("Client disconnected.");
        connectToServer();
        handshakeWithServer();
    }

    // wait to fully let the client disconnect
    delay(3000);
}

/**
 * @brief   Connect to server
 * @detail  Function to connect to server
 * @param   void
 * @return  void
 * @throws
 */
void connectToServer(void)
{
    // Connect to the websocket server
    if (client.connect(server_dns, SERVER_PORT))
    {
        SERIAL_DEBUG.println("Connected");
    }
    else
    {
        SERIAL_DEBUG.println("Connection failed.");
    }

}

/**
 * @brief   Handshake with the server
 * @detail  Function to handshake with the server(websocket server)
 * @param   void
 * @return  void
 * @throws
 */
void handshakeWithServer(void)
{
    // Handshake with the server
    webSocketClient.path = "/";
    webSocketClient.host = "echo.websocket.org";

    if (webSocketClient.handshake(client))
    {
        SERIAL_DEBUG.println("Handshake successful");
    }
    else
    {
        SERIAL_DEBUG.println("Handshake failed.");
        client.stop();
    }
}

/**
 * @brief   Initialize WizFi310
 * @detail	Function to initialize WizFi310 chip to use WIFI
 * @param	void
 * @return	void
 * @throws
 */
void initWizFi310()
{
    SERIAL_WIFI.begin(115200);

    WiFi.init(&SERIAL_WIFI);

    // check for the presence of the shield
    if (WiFi.status() == WL_NO_SHIELD)
    {
        SERIAL_DEBUG.println("[WIFI] WiFi shield not present");
        // don't continue
        while (true)
            ;
    }

    // attempt to connect to WiFi network
    while (status != WL_CONNECTED)
    {
        SERIAL_DEBUG.print("[WIFI] Attempting to connect to WPA SSID: ");
        SERIAL_DEBUG.println(ssid);
        // Connect to WPA/WPA2 network
        status = WiFi.begin(ssid, pass);
    }
    printWifiStatus();
}

/**
 * @brief   Print Wifi Status
 * @detail	Print Wifi Status using Serial : SSID, Allocated IP Address, Signal Strength
 * @param	void
 * @return	void
 * @throws
 */
void printWifiStatus()
{
    // print the SSID of the network you're attached to
    SERIAL_DEBUG.print("[WIFI] SSID: ");
    SERIAL_DEBUG.println(WiFi.SSID());

    // print your WiFi shield's IP address
    IPAddress ip = WiFi.localIP();
    SERIAL_DEBUG.print("[WIFI] IP Address: ");
    SERIAL_DEBUG.println(ip);

    // print the received signal strength
    long rssi = WiFi.RSSI();
    SERIAL_DEBUG.print("[WIFI] Signal strength (RSSI):");
    SERIAL_DEBUG.print(rssi);
    SERIAL_DEBUG.println(" dBm");
}
