/**************************************************************
 *
 * Coder: The Fabrication, REKA
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


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//-----------------------------------------------------------------------------------------------

// GPIO Setting
#define LED_OUTPUT 8
#define SWITCH_INPUT 9
#define Channel "startiot" // Replace with your channel name
#define Resource "res" //Replace with your resource name

// Your Host MQTT settings
const char* Host = "mqtt.beebotte.com";// your host broker URL
const char* Username = "token:token_7zZxK1sZM9Zb2vaG";// your username
const char* Password = "";// your password


//-----------------------------------------------------------------------------------------------
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



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
  
  pinMode(LED_OUTPUT,OUTPUT);
  pinMode(SWITCH_INPUT,INPUT_PULLUP);
  
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
  return mqtt.connected();
}

void loop() {

  if (mqtt.connected()) {  
    gopublish();
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
    root["resource"] = Resource;
    root["write"] = true;
    root["data"] = "hello world! Im RIG";

    // Now print the JSON into a char buffer
    char buffer[128];
    root.printTo(buffer, sizeof(buffer));

    // Create the topic to publish to
    char topic[64];
    sprintf(topic, "%s/%s", Channel, Resource);

    if (digitalRead(SWITCH_INPUT) == LOW) {
      // turn LED on:
      digitalWrite(LED_OUTPUT, HIGH);
      // Now publish the char buffer to Beebotte
      mqtt.publish(topic, buffer);
      Serial.println("Hello World. I'm RIG!!");
      delay(2000);
    } else {
      // turn LED off:
      digitalWrite(LED_OUTPUT, LOW);
    }

}



