
/*
 * create by Flo 0n 5.4.2021
 * based on multiple_accessories.ino
 *
 *  Created by Mixiaoxiao (Wang Bin)
 * 
 * Add 4 sensors to Homekit, two for Temp and humidity  inside
 * and two for temp and humidity remote
 * and one Switch for open the roof.
 
*/

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "Settings/wifi_info.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <DHT.h>
#include <RTClib.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <time.h>

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);
#define dataPin D3 // DHT22 sensor
#define DHTTYPE DHT22 // Creates a DHT object
DHT dht(dataPin, DHTTYPE);
#define RTC_I2C_ADDRESS 0x68 // I2C Adresse des RTC  DS3231
#define MY_NTP_SERVER "at.pool.ntp.org"           
#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3" 

U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

RF24 radio(D4, D8); // CE, CSN
const byte address[6] = "00001";

static const uint64_t pipes[6] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL, 0xF0F0F0F0B4LL, 0xF0F0F0F0A5LL, 0xF0F0F0F096LL};

String inTemp, inHum, outTemp, outHum;
typedef struct {
  float outdoorTemperatue;
  float outdoorHumidity;
  bool isRoofOpen;
}OutdoorWeather;

OutdoorWeather data;

bool isRoofOpen;
float indoorTemp, indoorHum, outdoorTemp, outdoorHum;
int draw_state = 0, doW;
unsigned long previousMillis = 0;
long interval = 3000;
time_t now; 
tm tm;


#define Temperature_20Icon_width 27
#define Temperature_20Icon_height 47
static const unsigned char Temperature_20Icon_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x80, 0x7f, 0x00, 0x00,
  0xc0, 0xe1, 0x00, 0x00, 0xe0, 0xc0, 0x01, 0x00, 0x60, 0x80, 0xf9, 0x03,
  0x60, 0x80, 0x01, 0x00, 0x60, 0x80, 0x01, 0x00, 0x60, 0x80, 0x79, 0x00,
  0x60, 0x80, 0x01, 0x00, 0x60, 0x80, 0x01, 0x00, 0x60, 0x80, 0xf9, 0x03,
  0x60, 0x80, 0x01, 0x00, 0x60, 0x80, 0x01, 0x00, 0x60, 0x8c, 0x79, 0x00,
  0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0xf9, 0x03,
  0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x79, 0x00,
  0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0xf9, 0x03,
  0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x01, 0x00, 0x60, 0x9e, 0x01, 0x00,
  0x70, 0x9e, 0x03, 0x00, 0x38, 0x1e, 0x07, 0x00, 0x18, 0x3e, 0x0e, 0x00,
  0x1c, 0x3f, 0x0c, 0x00, 0x0c, 0x7f, 0x18, 0x00, 0x8c, 0xff, 0x18, 0x00,
  0x8e, 0xff, 0x38, 0x00, 0xc6, 0xff, 0x31, 0x00, 0xc6, 0xff, 0x31, 0x00,
  0xc6, 0xff, 0x31, 0x00, 0x8e, 0xff, 0x38, 0x00, 0x8c, 0xff, 0x18, 0x00,
  0x0c, 0x7f, 0x1c, 0x00, 0x3c, 0x1c, 0x0e, 0x00, 0x78, 0x00, 0x06, 0x00,
  0xe0, 0x80, 0x07, 0x00, 0xe0, 0xff, 0x03, 0x00, 0x80, 0xff, 0x00, 0x00,
  0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


#define Humidity_20Icon_width 27
#define Humidity_20Icon_height 47
static const unsigned char Humidity_20Icon_bits[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00,
  0x00, 0x70, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00, 0xdc, 0x00, 0x00,
  0x00, 0xdc, 0x01, 0x00, 0x00, 0x8e, 0x01, 0x00, 0x00, 0x86, 0x03, 0x00,
  0x00, 0x06, 0x03, 0x00, 0x00, 0x03, 0x07, 0x00, 0x80, 0x03, 0x06, 0x00,
  0x80, 0x01, 0x0c, 0x00, 0xc0, 0x01, 0x1c, 0x00, 0xc0, 0x00, 0x18, 0x00,
  0xe0, 0x00, 0x38, 0x00, 0x60, 0x00, 0x30, 0x00, 0x70, 0x00, 0x70, 0x00,
  0x30, 0x00, 0xe0, 0x00, 0x38, 0x00, 0xc0, 0x00, 0x18, 0x00, 0xc0, 0x01,
  0x1c, 0x00, 0x80, 0x01, 0x0c, 0x00, 0x80, 0x03, 0x0e, 0x00, 0x80, 0x03,
  0x06, 0x00, 0x00, 0x03, 0x06, 0x00, 0x00, 0x03, 0x07, 0x00, 0x00, 0x07,
  0x03, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x06, 0x03, 0x00, 0x00, 0x06,
  0x63, 0x00, 0x00, 0x06, 0x63, 0x00, 0x00, 0x06, 0x63, 0x00, 0x00, 0x06,
  0xe3, 0x00, 0x00, 0x06, 0xc7, 0x00, 0x00, 0x06, 0xc6, 0x01, 0x00, 0x07,
  0x86, 0x03, 0x00, 0x03, 0x0e, 0x1f, 0x00, 0x03, 0x0e, 0x1e, 0x80, 0x01,
  0x1c, 0x00, 0xc0, 0x01, 0x38, 0x00, 0xe0, 0x00, 0x78, 0x00, 0x70, 0x00,
  0xf0, 0x00, 0x38, 0x00, 0xe0, 0x07, 0x1f, 0x00, 0x80, 0xff, 0x0f, 0x00,
  0x00, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
};

void setup() {
	Serial.begin(115200);
	wifi_connect(); // in wifi_info.h
	Wire.begin();
  radio.begin();
  delay(20);
  radio.setChannel(110);                
  radio.setAutoAck(0);
  radio.setRetries(15, 0);
  radio.setPALevel(RF24_PA_LOW);    
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
  u8g2.begin();
  dht.begin();
  configTime(MY_TZ, MY_NTP_SERVER);
	  //homekit_storage_reset();// to remove the previous HomeKit pairing storage when you first run this new HomeKit example
	my_homekit_setup();
  
}

void loop() {


	if (radio.available()) {
    
    radio.read(&data, sizeof(data)); // Read incoming data

    outdoorTemp = data.outdoorTemperatue;
    outdoorHum = data.outdoorHumidity;
    isRoofOpen = data.isRoofOpen;
  
  } 

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis > interval) {
    previousMillis = currentMillis;
    u8g2.firstPage();
    do {
      switch (draw_state ) {
        case 0: drawDate(); break;
        case 1: drawInTemperature(); break;
        case 2: drawInHumidity(); break;
        case 3: drawOutTemperature(); break;
        case 4: drawOutHumidity(); break;
      }
    } while ( u8g2.nextPage() );
    draw_state++;
    if (draw_state > 4) {
      draw_state = 0;
    }
    if (draw_state == 4) {
      sendData();
    }
  }
  my_homekit_loop();
	delay(10);
}

//==============================
// HomeKit setup and loop
//==============================

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_indoorTemperature;
extern "C" homekit_characteristic_t cha_indoorHumidity;
extern "C" homekit_characteristic_t cha_outdoorTemperature;
extern "C" homekit_characteristic_t cha_outdoorHumidity;
extern "C" homekit_characteristic_t cha_switch_isOpen;

// Called when the value is read by iOS Home APP
homekit_value_t cha_programmable_switch_event_getter() {
	// Should always return "null" for reading, see HAP section 9.75
	return HOMEKIT_NULL_CPP();
}

//Called when the switch value is changed by iOS Home APP
void cha_switch_on_setter(const homekit_value_t value) {
	bool isOpen = value.bool_value;
  Serial.println(value.bool_value);
  isRoofOpen = isOpen;	//sync the value
	sendData();  

}


void my_homekit_setup() {
  cha_switch_isOpen.setter = cha_switch_on_setter;
	arduino_homekit_setup(&config);
}

static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;

void my_homekit_loop() {
	arduino_homekit_loop();
	const uint32_t t = millis();
	if (t > next_report_millis) {
		// report sensor values every 10 seconds
		next_report_millis = t + 10 * 1000;
		my_homekit_report();
	}
	if (t > next_heap_millis) {
		// Show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}

void my_homekit_report() {

	cha_indoorTemperature.value.float_value = indoorTemp;
	homekit_characteristic_notify(&cha_indoorTemperature, cha_indoorTemperature.value);

	cha_indoorHumidity.value.float_value = indoorHum;
	homekit_characteristic_notify(&cha_indoorHumidity, cha_indoorHumidity.value);

	cha_outdoorTemperature.value.float_value = outdoorTemp;
	homekit_characteristic_notify(&cha_outdoorTemperature, cha_outdoorTemperature.value);

	cha_outdoorHumidity.value.float_value = outdoorHum;
	homekit_characteristic_notify(&cha_outdoorHumidity, cha_outdoorHumidity.value);

  cha_switch_isOpen.value.bool_value = isRoofOpen;
  homekit_characteristic_notify(&cha_switch_isOpen, cha_switch_isOpen.value);
	
	LOG_D("t %.1f, h %.1f", indoorTemp, indoorHum);

}

void sendData() {
  Serial.println("Sending");
  radio.stopListening();

  Serial.println(isRoofOpen);
  Serial.println(outdoorHum);
  Serial.println(outdoorTemp);
  data.isRoofOpen = isRoofOpen;
  data.outdoorHumidity = outdoorHum;
  data.outdoorTemperatue = outdoorTemp;
  radio.write(&data, sizeof(data));
  radio.startListening();
}


void drawInTemperature() {
indoorTemp = 0.0;
indoorTemp = dht.readTemperature();

 
  int t = int(indoorTemp);
  inTemp = String(t)  + char(176) + "C";
  u8g2.setFont(u8g2_font_helvR14_tr);
  u8g2.setCursor(24, 15);
  u8g2.print("INDOOR");
  u8g2.setFont(u8g2_font_fub30_tf);
  u8g2.setCursor(36, 58);
  u8g2.print(inTemp);
  u8g2.drawXBMP( 1, 17, Temperature_20Icon_width, Temperature_20Icon_height, Temperature_20Icon_bits);
}

void drawInHumidity() {
indoorHum = 0.0;
indoorHum = dht.readHumidity();

int h = int(indoorHum);
  inHum = String(h) + "%";
  u8g2.setFont(u8g2_font_helvR14_tr);
  u8g2.setCursor(24, 15);
  u8g2.print("INDOOR");
  u8g2.setFont(u8g2_font_fub30_tf);
  u8g2.setCursor(36, 58);
  u8g2.print(inHum);
  u8g2.drawXBMP( 1, 17, Humidity_20Icon_width, Humidity_20Icon_height, Humidity_20Icon_bits);
}

void drawOutTemperature() {
  int outT = int(outdoorTemp);
  outTemp = String(outT);
  u8g2.setFont(u8g2_font_helvR14_tr);
  u8g2.setCursor(24, 15);
  u8g2.print("OUTDOOR");
  u8g2.setFont(u8g2_font_fub30_tf);
  u8g2.setCursor(36, 58);
  u8g2.print(outTemp + char(176) + "C");
  u8g2.drawXBMP( 1, 17, Temperature_20Icon_width, Temperature_20Icon_height, Temperature_20Icon_bits);
}
void drawOutHumidity() {
  int outH = int(outdoorHum);
  outHum = String(outH);
  u8g2.setFont(u8g2_font_helvR14_tr);
  u8g2.setCursor(12, 15);
  u8g2.print("OUTDOOR");
  u8g2.setFont(u8g2_font_fub30_tf);
  u8g2.setCursor(36, 58);
  u8g2.print(outHum + "%");
  u8g2.drawXBMP( 1, 17, Humidity_20Icon_width, Humidity_20Icon_height, Humidity_20Icon_bits);
}

void  drawDate(){
  time(&now);                       // read the current time
  localtime_r(&now, &tm);
  String DoWasText = "";
 switch (tm.tm_wday) {
        case 0: DoWasText = "SONNTAG"; break;
        case 1: DoWasText = "MONTAG"; break;
        case 2: DoWasText = "DIENSTAG"; break;
        case 3: DoWasText = "MITTWOCH"; break;
        case 4: DoWasText = "DONNERSTAG"; break;
        case 5: DoWasText = "FREITAG"; break;
        case 6: DoWasText = "SAMSTAG"; break;
      } 
  Serial.println(DoWasText);
  
  u8g2.setFont(u8g2_font_helvR14_tr);
  u8g2.setCursor(12, 15);
  u8g2.print(DoWasText);
  u8g2.setCursor(12, 35);
  u8g2.print(tm.tm_mday, DEC);
  u8g2.print(".");
    u8g2.print(tm.tm_mon + 1, DEC);
  u8g2.print(".");
  u8g2.print(tm.tm_year+ 1900, DEC);
  u8g2.setCursor(12, 55);
  u8g2.print(tm.tm_hour, DEC);
  u8g2.print(" : ");
  u8g2.print(tm.tm_min, DEC);
}