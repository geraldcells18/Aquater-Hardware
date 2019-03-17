#include <SoftwareSerial.h>

#define VALVE_PIN 4

SoftwareSerial wemos(5, 6); // Rx - Tx

byte sensorInterrupt = 0; //Interrupt pin for the process of interrupt function.
byte sensorPin = 2; //Water flow sensor pin for the water measuring.

float calibrationFactor = 6.5; //This object used for water measuring (6.5 revolution per mL)
volatile int pulseCount = 0; //This object used to get the water flow sensor pulse count.

float flowRate = 0.0; //This object used for fetching the current flow of the water.
long flowMilliLitres = 0; //This object used for storing the current flowed mL.
long totalMilliLitres = -1; //This object used for storing the cumulative mL.

unsigned long oldTime = 0; //This object used to hold the old millis.

unsigned long liters = 0; //This object may vary and used for storing of cumulative total of liters.
unsigned long old_liters = 0; //This object used to store the old Liters.

float cm3 = 0.00; //This object may vary and used for storing the cumulative total of cubic meters.

int LDR_READING = 0;
String valve_status = "ON";

void getClarity() {
	LDR_READING = analogRead(A7);
}

void valveRoutine() {
	if (wemos.available()) {
		String valve_state = wemos.readStringUntil('\n');
		valve_state.trim();
		toggleValve(valve_state);
	}
}

void toggleValve(String state) {
	if (state == "ON" || state == "OFF") {
		if (state == "ON") {
			digitalWrite(VALVE_PIN, HIGH);
			digitalWrite(LED_BUILTIN, HIGH);
			valve_status = state;
		}
		else {
			digitalWrite(VALVE_PIN, LOW);
			digitalWrite(LED_BUILTIN, LOW);
			valve_status = state;
		}
	}
}

void setup() {

	Serial.begin(9600);
	wemos.begin(9600);
	wemos.setTimeout(300);

	delay(500);

	pinMode(VALVE_PIN, OUTPUT);
	digitalWrite(VALVE_PIN, HIGH);
	pinMode(LED_BUILTIN, OUTPUT);

	pinMode(sensorPin, INPUT);
	digitalWrite(sensorPin, HIGH);

	attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

	delay(10000);

	while (totalMilliLitres == -1) {
		if (wemos.available()) {
			String data = wemos.readStringUntil('\n');
			data.trim();
			data.replace(" ", "");
			if (data.toInt() >= 0) {
				wemos.println("r");
				totalMilliLitres = data.toInt();
			}
		}

		delay(500);
	}
}

//Pulse method
void pulseCounter()
{
	pulseCount++;
}

//Water flow process.
void mainProc()
{
	detachInterrupt(sensorInterrupt);

	flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
	oldTime = millis();

	flowMilliLitres = (flowRate / 60) * 1000;
	totalMilliLitres += flowMilliLitres;

	liters = totalMilliLitres / 1000;
	cm3 = liters / 1000.0;

	pulseCount = 0;
	attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}

void sendData() {
	wemos.println(String(liters) + "," + String(cm3) + "," + String(LDR_READING) + ",");
	Serial.println(String(liters) + "," + String(cm3) + "," + String(LDR_READING) + "," + valve_status + ",");
}

void loop() {
	if (millis() - oldTime > 1000) {
		mainProc();
		getClarity();
		sendData();
	}

	valveRoutine();
}
