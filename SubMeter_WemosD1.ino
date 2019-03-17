#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <CellsCSVParser.h>

#define LED_PIN D6
#define BUZZER_PIN D7
#define BUTTON_PIN D5

SoftwareSerial arduino(D2, D3);
DynamicJsonBuffer jsonBuffer(512);

IPAddress Access_Point_IP(192, 168, 4, 1);
ESP8266WebServer server(80);
  
byte once = 0;
byte once_wifi_piezo = 0;
bool enabledClient = false;

String last_liters = "0";
String _response = "";

HTTPClient http;

String IP_ADDRESS = "http://192.168.43.80/";

void setConsumption(String liters, String cubic) {
	if (liters != "" && cubic != "") {
		digitalWrite(LED_PIN, LOW);
		if (WiFi.status() == WL_CONNECTED) {
			http.begin(IP_ADDRESS + "/aquater/last-day-data.php");
			int httpCode = http.GET();
			if (httpCode > 0) {
				digitalWrite(LED_PIN, HIGH);

				String response = http.getString();
				response.trim();
				response.replace(" ", "");
				http.end();

				if (response == "0") {
					http.begin(IP_ADDRESS + "/aquater/insert-data.php?liters=" + liters + "&cubic=" + cubic);
					http.GET();
					http.end();
					last_liters = liters;
				}
				else {
					JsonObject& root = jsonBuffer.parseObject(response);
					if (root.success()) {

						String var_liters = root["liters"].as<String>();

						if (liters != last_liters) {

							uint32_t liters_diff = liters.toInt() - last_liters.toInt();
							uint32_t liters_sum = liters_diff + var_liters.toInt();

							float cubic_sum = (liters_sum / 1000.0);

							http.begin(IP_ADDRESS + "/aquater/insert-data.php?liters=" + String(liters_sum) + "&cubic=" + String(cubic_sum, 2));
							http.GET();
							http.end();
							last_liters = liters;
						}
					}
				}
			}
		}
	}
}

void setClarityData(String clarity) {
	if (clarity != "" && clarity != "0") {
		digitalWrite(LED_PIN, LOW);
		if (WiFi.status() == WL_CONNECTED) {
			http.begin(IP_ADDRESS + "/aquater/water-clarity/set-clarity.php?clarity=" + clarity);
			int httpCode = http.GET();
			if (httpCode > 0) {
				digitalWrite(LED_PIN, HIGH);
			}
		}
	}

	http.end();
}

void getValveStatOnline() {
	digitalWrite(LED_PIN, LOW);
	if (WiFi.status() == WL_CONNECTED) {
		http.begin(IP_ADDRESS + "/aquater/water-switch/get-valve.php");
		int httpCode = http.GET();
		if (httpCode > 0) {
			digitalWrite(LED_PIN, HIGH);

			String response = http.getString();
			response.trim();
			response.replace(" ", "");

			if (response == "1") {
				arduino.println("ON");
				if (once_wifi_piezo == 0) {
					digitalWrite(BUZZER_PIN, HIGH);
					delay(500);
					digitalWrite(BUZZER_PIN, LOW);
					once_wifi_piezo = 1;
				}
			}
			else {
				arduino.println("OFF");
				once_wifi_piezo = 0;
			}
		}
	}
	http.end();
}

void getValveStatOffline(String state) {
	if (enabledClient) {
		server.handleClient();
		if (state == "1") {
			arduino.println("ON");
			if (once == 0) {
				digitalWrite(BUZZER_PIN, HIGH);
				delay(500);
				digitalWrite(BUZZER_PIN, LOW);
				once = 1;
			}
		}
		else {
			arduino.println("OFF");
			once = 0;
		}
	}
}

byte buttonRead() {
	byte btn_reading = digitalRead(BUTTON_PIN);
	if (btn_reading == HIGH) {
		return 1;
	}
	else {
		return 0;
	}
}

void sendDataToServer(String water_data) {
	CellsCSVParser csv(water_data);
	if (csv.getCSVDataCount() == 4) {
		setClarityData(csv.parseCSV(3));
		setConsumption(csv.parseCSV(1), csv.parseCSV(2));
	}
}

void getData()
{
	if (arduino.available()) {
		String data = arduino.readStringUntil('\n');
		data.trim();
		data.replace(" ", "");
		sendDataToServer(data);
	}
}

void getCallBackToArduino() {
	if (arduino.available()) {
		_response = arduino.readStringUntil('\n');
		_response.trim();
		_response.replace(" ", "");
	}
}

void getLastLiters() {
	digitalWrite(LED_PIN, LOW);
	if (WiFi.status() == WL_CONNECTED) {
		http.begin(IP_ADDRESS + "/aquater/last-data.php");
		int httpCode = http.GET();
		if (httpCode > 0) {
			digitalWrite(LED_PIN, HIGH);

			String response = http.getString();
			response.trim();
			response.replace(" ", "");

			if (response == "0") {
				arduino.println("0");
				getCallBackToArduino();
			}

			else {
				JsonObject& root = jsonBuffer.parseObject(response);
				if (root.success()) {

					String liters_from_db = root["liters"].as<String>();

					last_liters = liters_from_db;
					uint32_t mL = liters_from_db.toInt() * 1000;
					arduino.println(mL);

					getCallBackToArduino();
				}
			}
		}
	}

	http.end();
}

void SendToClient() {

	server.send(200, "text/html", "connected");
	String ssid = server.arg("ssid");
	String password = server.arg("password");

	ssid.trim();
	password.trim();

	Serial.println(ssid);
	Serial.println(password);

	WiFi.mode(WIFI_OFF);
	delay(2000);
	WiFi.mode(WIFI_STA);
	delay(2000);

	WiFi.begin(ssid.c_str(), password.c_str());

	digitalWrite(LED_PIN, HIGH);
	delay(8000);
	digitalWrite(LED_PIN, LOW);

	delay(1000);
	ESP.restart();
}

void setESPConnectionMode(byte mode) {
	if (mode == 1) {
		WiFi.enableAP(false);
		WiFi.enableSTA(true);
	}
	if (mode == 2) {
		WiFi.enableAP(true);
		WiFi.enableSTA(false);
	}
}

void setup()
{
	Serial.begin(9600);
	arduino.begin(9600);
	arduino.setTimeout(300);

	pinMode(LED_PIN, OUTPUT);
	pinMode(BUZZER_PIN, OUTPUT);
	pinMode(BUTTON_PIN, INPUT_PULLUP);

	WiFi.mode(WIFI_OFF);
	delay(1000);
	WiFi.mode(WIFI_STA);

	WiFi.begin();

	setESPConnectionMode(1);

	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);

	delay(8000);

	if (WiFi.status() == WL_CONNECTED) {

		digitalWrite(LED_PIN, HIGH);
		digitalWrite(BUZZER_PIN, HIGH);
		delay(200);
		digitalWrite(LED_PIN, LOW);
		digitalWrite(BUZZER_PIN, LOW);
		delay(200);
		digitalWrite(LED_PIN, HIGH);
		digitalWrite(BUZZER_PIN, HIGH);
		delay(200);
		digitalWrite(LED_PIN, LOW);
		digitalWrite(BUZZER_PIN, LOW);

		while (_response != "r") {
			getLastLiters();
			delay(1000);
		}
	}
	else {
		
		digitalWrite(LED_PIN, HIGH);

		arduino.println("0");
		getCallBackToArduino();
		delay(1000);

		WiFi.mode(WIFI_OFF);
		delay(1000);
		WiFi.mode(WIFI_AP);

		setESPConnectionMode(2);

		WiFi.softAPConfig(Access_Point_IP, Access_Point_IP, IPAddress(255, 255, 255, 0));
		WiFi.softAP("Aquater Device", "");
		server.on("/", SendToClient);
		server.begin();

		enabledClient = true;
	}
}

void loop()
{
	getData();
	getValveStatOnline();
	getValveStatOffline(String(buttonRead()));
	delay(500);
}