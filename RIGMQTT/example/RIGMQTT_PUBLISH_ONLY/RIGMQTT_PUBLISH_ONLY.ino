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


// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "";
const char user[] = "";
const char pass[] = "";


const char* broker = "mciot.imbig.com.my";//broker URL or IP
const char* topicMachine = "$imbig/Machine";//Topic at brker

//RIG CELL LITE MQTT SETUP PARAMETER
#define GSM_STATUS 6
#define GSM_PWRKEY 7
#define GSM_TXD 4
#define GSM_RXD 2
SoftwareSerial SerialAT(GSM_TXD, GSM_RXD);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);



//PIN STATUS TO BE PUBLISHED
#define PIN8 8
#define PIN9 9
#define PIN10 10



long lastReconnectAttempt = 0;

void setup() {
  
  pinMode(PIN8,INPUT_PULLUP);
  pinMode(PIN9,INPUT_PULLUP);
  pinMode(PIN10,INPUT_PULLUP);

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
  mqtt.setServer(broker, 1883);
}

boolean mqttConnect() {
  Serial.print("Try to connect to ");
  Serial.print(broker);
  if (!mqtt.connect("RCL","admin","P@ssw0rd")) //Broker ID&Password
  {
    Serial.println(" fail");
    return false;
  }
  Serial.println(" OK");
  return mqtt.connected();
}

void loop() {

  if (mqtt.connected()) {
    mqttpublish();
    mqtt.loop();
  } else {
    // Reconnect every 10 seconds when connection lost
    unsigned long t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }

}

void mqttpublish() {

char out[5];
  // put your main code here, to run repeatedly:
int s1 = digitalRead(PIN8);
int s2 = digitalRead(PIN9);
int s3 = digitalRead(PIN10);

sprintf(out, "%d%d%d", s1,s2,s3);

String b = String(s1)+String(s2)+String(s3);


  mqtt.publish(topicMachine, out);
  Serial.println(b);
  delay(3000);//publish delay 3s

} 

