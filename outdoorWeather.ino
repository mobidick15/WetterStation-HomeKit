#include <SD.h>

/* Regente Florian

Outdoor weather modul 


*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <DHT.h>
#include <Servo.h>

#define DHTPIN 8 // Ich nutze Digital-PIN 8
#define DHTTYPE DHT22 // DHT 22 (AM2302)
#define POWERPIN 4


DHT dht(DHTPIN, DHTTYPE);
RF24 radio(10, 9); // CE, CSN

static const uint64_t pipes[6] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL, 0xF0F0F0F0C3LL, 0xF0F0F0F0B4LL, 0xF0F0F0F0A5LL, 0xF0F0F0F096LL};

bool isOpen = false;
typedef struct {
  float outdoorTemperatue;
  float outdoorHumidity;
  bool isRoofOpen;
}OutdoorWeather;

OutdoorWeather data;

Servo myservo;


void setup() {
  pinMode(POWERPIN, OUTPUT); 
  digitalWrite(POWERPIN, LOW);
  radio.begin();
  delay(20);
  radio.setChannel(110);                
  radio.setAutoAck(0);
  radio.setPALevel(RF24_PA_LOW);     
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);
  radio.startListening();
  Serial.begin(9600);
  dht.begin();
  myservo.attach(2);//connect pin 2 with the control line(the middle line of Servo)  

}
void loop() {
recieveData();
}

void sendData(){
  Serial.println("Senden:");
  Serial.println(isOpen);
  Serial.println(dht.readHumidity());
  Serial.println(dht.readTemperature());
  data.isRoofOpen = isOpen;
  data.outdoorHumidity = dht.readHumidity();
  data.outdoorTemperatue = dht.readTemperature();

  radio.stopListening();
  radio.write(&data, sizeof(data));
  radio.startListening();
}

void recieveData(){
 
  if (radio.available()) {
    Serial.println("Empfange"); 
    radio.read(&data, sizeof(data)); // Read incoming data

    Serial.println(data.isRoofOpen);
    isOpen = data.isRoofOpen;
    Serial.println(data.outdoorTemperatue); 
    delay(10); 
    if(isOpen){
      openServo();
    }else {
      closeServo();
    }
    sendData();
  } 
}


void openServo(){
  digitalWrite(POWERPIN, HIGH);
  Serial.println("Open"); 
  myservo.write(180);
  isOpen = true;
  delay(20);
  digitalWrite(POWERPIN, LOW);
 
}

void closeServo(){
  digitalWrite(POWERPIN, HIGH);
  Serial.println("close");
  myservo.write(85);
  isOpen = false;
  delay(20);
  digitalWrite(POWERPIN, LOW);
}
