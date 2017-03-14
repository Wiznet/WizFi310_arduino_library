/**
 * @project WizFi310 Post Sensing Data (feat. Dweet.io)
 * @brief   Post to cloud(Dweet.io) your sensor's data using WizFi310
 * @detail  refer to http://wiznet.io
 * @author  Kei ( http://openmaker.tistory.com/ )
 * @date    2017-02-17
 * @version 0.0.1
 * @Borad   Arduino MEGA 2560 (MCU : Atmel ATmega2560)
 */

/* Includes ------------------------------------------------------------------*/
#include <Arduino.h>
#include "WizFi310.h"

/* Private variables ---------------------------------------------------------*/
#define POST_DATA_INTERVAL  20000  //  1000ms = 1s(sec)
#define PIN_RESISTOR         A0
#define PIN_LIGHT            A1
#define PIN_TEMP             A2
#define SERIAL_WIFI  Serial3
#define SERIAL_DEBUG Serial

char ssid[] = "YOURSSID";    // your network SSID (name)
char pass[] = "YOURSSIDPASSWORD";          // your network password
int status = WL_IDLE_STATUS;       // the Wifi radio's status

const char* server_dns = "www.dweet.io";
const char* post_url = "POST /dweet/for/YOURTHINGSNAME?";	// Change YOURTHINGSNAME
//const char* post_url = "POST /dweet/for/academytest?"; //example

WiFiClient client;

/* Private function prototypes -----------------------------------------------*/

void initSensor(void);
int calculateTemp(byte sensor_pin);
int postAnalogSensor(void);
void debugHTTPResponse();
void initWizFi310(void);
void printWifiStatus(void);

void setup() {
	SERIAL_DEBUG.begin(115200);
	while (!SERIAL_DEBUG); //for test delay

	initSensor();

	SERIAL_DEBUG.println("*****  WizFi310 & Mega Post Dweet.io *****");

	initWizFi310();
}

void loop() {
	static uint32_t tTime;

	if ((millis() - tTime) >= POST_DATA_INTERVAL)    
	{
		if (postAnalogSensor()) {
		//	debugHTTPResponse();	//for debug
		}
		tTime = millis();
		client.stop();
	}
}

/**
 * @brief   Post analog sensor data
 * @detail	Post to dweet analog sensor data using HTTP.
 * @param	void
 * @return	connect result
 * @throws
 */
int postAnalogSensor(void) {
	SERIAL_DEBUG.println("[Net Info] connecting to server...");

	if (client.connect(server_dns, 80)) {
		SERIAL_DEBUG.print("[Net Info] Connected! Sending HTTP request to ");
		SERIAL_DEBUG.println(server_dns);

		client.print(post_url);

		client.print("RESIS=");
		client.print(analogRead(PIN_RESISTOR));
		client.print("&LIGHT=");
		client.print(analogRead(PIN_LIGHT));
		client.print("&TEMP=");
		client.println(calculateTemp(PIN_TEMP));

		client.print("Host: ");
		client.println(server_dns);
		client.println("Connection: close");
		client.println();

		//SERIAL_DEBUG.println("Send Request !");

		SERIAL_DEBUG.println();

		return 1;
	}

	SERIAL_DEBUG.println("[Net Info] Connect Failed!");
	SERIAL_DEBUG.println();
	return 0;
}

/**
 * @brief   Debug HTTP Response
 * @detail	After request, check HTTP respose about request
 * @param	void
 * @return	void
 * @throws
 */
void debugHTTPResponse() {
	delay(500);  // If need receive delay, use this.
	while (client.connected()) {
		if (client.available())
			SERIAL_DEBUG.print((char) client.read());
	}
	SERIAL_DEBUG.println();
	SERIAL_DEBUG.println();
}

/**
 * @brief   Initialize sensor
 * @detail	Initialize each sensor
 * @param	void
 * @return	void
 * @throws
 */
void initSensor(void) {
	pinMode(PIN_RESISTOR, INPUT);
	pinMode(PIN_LIGHT, INPUT);
	pinMode(PIN_TEMP, INPUT);
}

/**
 * @brief   Calculate Temperature
 * @detail	Depend on sensor. this function used LM35
 * @param	byte sensor_pin Pin to which LM35 is connected
 * @return	(int) Temperature value (Celsius degree)
 * @throws
 */
int calculateTemp(byte sensor_pin) {
	float tmp_c = analogRead(sensor_pin);
	// http://playground.arduino.cc/Main/LM35HigherResolution
	tmp_c = (3.3 * tmp_c * 100.0) / 1024.0;

	return (int) tmp_c;
}

/**
 * @brief   Initialize WizFi310
 * @detail	Function to initialize WizFi310 chip to use WIFI
 * @param	void
 * @return	void
 * @throws
 */
void initWizFi310() {
	SERIAL_WIFI.begin(115200);

	WiFi.init(&SERIAL_WIFI);

	// check for the presence of the shield
	if (WiFi.status() == WL_NO_SHIELD) {
		SERIAL_DEBUG.println("[WIFI] WiFi shield not present");
		// don't continue
		while (true);
	}

	// attempt to connect to WiFi network
	while (status != WL_CONNECTED) {
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
void printWifiStatus() {
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
