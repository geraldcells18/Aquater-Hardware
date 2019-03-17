#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <CellsCSVParser.h>
#include <LiquidCrystal_I2C.h>

byte customCharCubic[8] = { 0b11100, 0b00100, 0b11100, 0b00100, 0b11100, 0b00000, 0b00000, 0b00000 };
byte customCharCheck[8] = { 0b00000, 0b00000, 0b00001, 0b00010, 0b10100, 0b01000, 0b00000, 0b00000 };

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

byte once = 0;
String old_switch = "";

void setup() {

	Serial.begin(9600);
	Serial1.begin(9600); // WiFi module UART
	Serial2.begin(9600); // HC-05 Bluetooth module UART
	delay(100);

	// Instantiate setTimeout method for setting the data streaming timeout/speed
	Serial1.setTimeout(100);
	Serial2.setTimeout(100);
	delay(100);

	// Instantiate 20x 4 LCD
	lcd.backlight();
	lcd.begin(20, 4);
	lcd.setCursor(6, 1);
	lcd.print("AquaTer");
}

// The loop function runs over and over again until power down or reset
void loop() {
	if (Serial2.available()) {
		
		String data = Serial2.readStringUntil('\n');
		data.trim();
		data.replace(" ", "");
		CellsCSVParser csv(data);
		Serial.println(data);
		Serial.println(String(csv.getCSVDataCount()));
		if (csv.getCSVDataCount() == 6) {
			if (once == 0) {
				lcd.clear();
				once = 1;
			}
			if (csv.parseCSV(4) != old_switch) {
				lcd.clear();
				old_switch = csv.parseCSV(4);
			}
			lcd.createChar(0, customCharCubic);
			lcd.setCursor(0, 0);
			lcd.print("Liters:" + csv.parseCSV(1) + "L");
			lcd.setCursor(0, 1);
			lcd.print("Cubic:" + String(csv.parseCSV(2) + "cm")); lcd.write((byte)0);
			lcd.setCursor(0, 2);
			lcd.print("Clarity:" + csv.parseCSV(3));
			lcd.setCursor(0, 3);
			lcd.print("Switch:" + csv.parseCSV(4));
		}
	}
}

