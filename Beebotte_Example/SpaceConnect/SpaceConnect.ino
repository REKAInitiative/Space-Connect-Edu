/**************************************************************
 *
 * Coder: A Perpetuator, REKA
 * Related Library, TINYGSM, PUBSUB
 * 
 *
 * Example explanation:
 *   This code is to only publish the status of 3 pins namely
 *   pin D8,D9,D10 of RIG Cell Lite. Outcome is a 3 char of 1
 *   and 0, ex: 000, 001, 011, etc every 3s
 *
 **************************************************************/

#include <RIGMQTT.h>
#include <ArduinoJson.h>

#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Wire.h> //Including wire library

#include <SFE_BMP180.h> //Including BMP180 library

#define ALTITUDE 0 //[CHANGE DURING LAUNCH]Altitude where I live (In nibong tebal 3feet/0meter)

SFE_BMP180 pressure; //Creating an object
SoftwareSerial mySerial(9, 10);
TinyGPS gps;

void gpsdump(TinyGPS &gps);
void printFloat(double f, int digits = 2);



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//-----------------------------------------------------------------------------------------------

// GPIO Setting
#define LEDPIN  9         // Pin which is connected to LED.
#define Channel "startiot" // Replace with your channel name
#define Resource1 "res" //Replace with your resource name
#define Resource2 "led" //Replace with your another resource name

// Your Host MQTT settings
const char* Host = "mqtt.beebotte.com";// your host broker URL
const char* Username = "token:token_7zZxK1sZM9Zb2vaG";// your username
const char* Password = "";// your password


//-----------------------------------------------------------------------------------------------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


const int analogInPin = A0;
int sensorValue = 0;  // value read from the sensor

// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "";
const char user[] = "";
const char pass[] = "";


//RIG CELL LITE MQTT SETUP PARAMETER
#define GSM_STATUS 6
#define GSM_PWRKEY 7
#define GSM_TXD 4
#define GSM_RXD 2
SoftwareSerial SerialAT(GSM_TXD, GSM_RXD);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);


long lastReconnectAttempt = 0;

void setup() {
  
  
  /**** RIG CELL LITE SETUP START *****/

  // Set RIG module baud rate
  SerialAT.begin(9600);
  
  pinMode(GSM_PWRKEY,OUTPUT);
  pinMode(GSM_STATUS,INPUT);
  
  digitalWrite(GSM_PWRKEY,LOW);
  if(!digitalRead(GSM_STATUS))
  {
    digitalWrite(GSM_PWRKEY,HIGH);
    while(1)
    {
      if(digitalRead(GSM_STATUS))
      {
        digitalWrite(GSM_PWRKEY,LOW);
        break;
      }
    }
  }

  /**** RIG CELL LITE SETUP END *****/
  
  // Set serial baud rate
  Serial.begin(9600);
  delay(2000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();

  // If SIM has password, Unlock your SIM card with a PIN
  //modem.simUnlock("1234");

  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    while (true);
  }
  Serial.println(" OK");

  Serial.print("Connecting to ");
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    while (true);
  }
  Serial.println(" OK");

  // Set MQTT Broker setup
  mqtt.setServer(Host, 1883);
  mqtt.setCallback(onMessage);
}

boolean mqttConnect() {
  Serial.print("Try to connect to ");
  Serial.print(Host);
  if (!mqtt.connect(generateID(),Username,Password)) //Broker ID&Password
  {
    Serial.println(" fail");
    return false;
  }
  Serial.println(" OK");
  char topic[64];
  sprintf(topic, "%s/%s", Channel, Resource2);
  mqtt.subscribe(topic);
  return mqtt.connected();
}

void loop() {

  if (mqtt.connected()) {  
    gopublish();
    mqtt.loop();
  } else {
    // Reconnect every 5 seconds when connection lost
    unsigned long t = millis();
    if (t - lastReconnectAttempt > 5000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }

}


void onMessage(char* topic, byte* payload, unsigned int length) {
  // decode the JSON payload
  StaticJsonBuffer<128> jsonInBuffer;
  JsonObject& root = jsonInBuffer.parseObject(payload);
  Serial.println("Someone is publishing to me..");
  // Test if parsing succeeds.
  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  // led resource is a boolean read it accordingly
  bool data = root["data"];

  // Set the led pin to high or low
  digitalWrite(LEDPIN, data ? HIGH : LOW);

  // Print the received value to serial monitor for debugging
  Serial.print("Received message of length ");
  Serial.print(length);
  Serial.println();
  Serial.print("data ");
  Serial.print(data);
  Serial.println();
}


const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
char id[17];

const char * generateID()
{
  randomSeed(analogRead(0));
  int i = 0;
  for(i = 0; i < sizeof(id) - 1; i++) {
    id[i] = chars[random(sizeof(chars))];
  }
  id[sizeof(id) -1] = '\0';

  return id;
}

// publishes data to the specified resource
void gopublish()
{
    StaticJsonBuffer<128> jsonOutBuffer;
    JsonObject& root = jsonOutBuffer.createObject();
    root["channel"] = Channel;
    root["resource"] = Resource1;
    root["write"] = true;
    sensorValue = analogRead(analogInPin);
    root["data"] = sensorValue;

    // Now print the JSON into a char buffer
    char buffer[128];
    root.printTo(buffer, sizeof(buffer));

    // Create the topic to publish to
    char topic[64];
    sprintf(topic, "%s/%s", Channel, Resource1);
    // Now publish the char buffer to Beebotte
    mqtt.publish(topic, buffer);
    Serial.println(sensorValue);
    delay(1000);
  

}

void gpsdump(TinyGPS &gps)
{
  long lat, lon;
  float flat, flon;
  unsigned long age, date, time, chars;
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned short sentences, failed;

  gps.get_position(&lat, &lon, &age);
  //Serial.print("Lat/Long(10^-5 deg): "); Serial.print(lat); Serial.print(", "); Serial.print(lon); 
  //Serial.print(" Fix age: "); Serial.print(age); Serial.println("ms.");
  
  // On Arduino, GPS characters may be lost during lengthy Serial.print()
  // On Teensy, Serial prints to USB, which has large output buffering and
  //   runs very fast, so it's not necessary to worry about missing 4800
  //   baud GPS characters.

  gps.f_get_position(&flat, &flon, &age);
  Serial.print(">Lat/Long(float): "); printFloat(flat, 5); Serial.print(", "); printFloat(flon, 5);
  // Serial.print(" Fix age: "); Serial.print(age); Serial.println("ms.");

  gps.get_datetime(&date, &time, &age);
  //Serial.print("Date(ddmmyy): "); Serial.print(date);
  
  //Serial.print(" Time(hhmmsscc): ");
  //Serial.print(time);
  //Serial.print(" Fix age: "); Serial.print(age); Serial.println("ms.");

  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  //Serial.print("Date: "); Serial.print(static_cast<int>(month)); Serial.print("/"); 
   // Serial.print(static_cast<int>(day)); Serial.print("/"); Serial.print(year);
  
  Serial.print(" >Time: "); Serial.print(static_cast<int>(hour+8));  Serial.print(":"); //Serial.print("UTC +08:00 Malaysia");
    Serial.print(static_cast<int>(minute)); Serial.print(":"); Serial.print(static_cast<int>(second));
    Serial.print("."); Serial.print(static_cast<int>(hundredths)); //Serial.print(" UTC +08:00 Malaysia");
  //Serial.print("  Fix age: ");  Serial.print(age); Serial.println("ms.");

  //Serial.print("Alt(cm): "); Serial.print(gps.altitude()); Serial.print(" Course(10^-2 deg): ");
    //Serial.print(gps.course()); Serial.print(" Speed(10^-2 knots): "); Serial.println(gps.speed());
  Serial.print(" >Alt(float): "); printFloat(gps.f_altitude()); 
  //Serial.print(" Course(float): ");
  //printFloat(gps.f_course()); Serial.println();
 
  //Serial.print("Speed(knots): "); printFloat(gps.f_speed_knots()); Serial.print(" (mph): ");
    //printFloat(gps.f_speed_mph());
  //Serial.print(" (mps): "); printFloat(gps.f_speed_mps()); 
  
  Serial.print("  >Speed(kmph): ");
    printFloat(gps.f_speed_kmph()); Serial.println();

  gps.stats(&chars, &sentences, &failed);
  //Serial.print("Stats: characters: "); Serial.print(chars); Serial.print(" sentences: ");
   // Serial.print(sentences); Serial.print(" failed checksum: "); Serial.println(failed);
}

void printFloat(double number, int digits)
{
  // Handle negative numbers
  if (number < 0.0) 
  {
     Serial.print('-');
     number = -number;
  }

  // Round correctly so that print(1.999, 2) prints as "2.00"
  double rounding = 0.5;
  for (uint8_t i=0; i<digits; ++i)
    rounding /= 10.0;
  
  number += rounding;

  // Extract the integer part of the number and print it
  unsigned long int_part = (unsigned long)number;
  double remainder = number - (double)int_part;
  Serial.print(int_part);

  // Print the decimal point, but only if there are digits beyond
  if (digits > 0)
    Serial.print("."); 

  // Extract digits from the remainder one at a time
  while (digits-- > 0) 
  {
    remainder *= 10.0;
    int toPrint = int(remainder);
    Serial.print(toPrint);
    remainder -= toPrint;
  }
}

