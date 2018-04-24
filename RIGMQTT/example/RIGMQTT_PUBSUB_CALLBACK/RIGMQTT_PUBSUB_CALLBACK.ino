/**************************************************************
 *
 * Coder: The Fabrication, REKA
 * Related Library, TINYGSM, PUBSUB
 * 
 *
 * Example explanation:
 *   This code is to subscribe a topic from MQTT broker.
 *   When the message from the topic is "ON" the LED will ON
 *   and when message is "OFF" the LED will OFF. The LED status
 *   is also published to the broker with different topic
 **************************************************************/

#include <RIGMQTT.h>



// Your GPRS credentials
// Leave empty, if missing user or pass
const char apn[]  = "";
const char user[] = "";
const char pass[] = "";

//MQTT info
const char* broker = "mciot.imbig.com.my";//broker URL or IP
const char* PubTopic = "$imbig/Pub";//Publish topic
const char* SubTopic = "$imbig/Sub";//Subscribe topic

//RIG CELL LITE MQTT SETUP PARAMETER
#define GSM_STATUS 6
#define GSM_PWRKEY 7
#define GSM_TXD 4
#define GSM_RXD 2
SoftwareSerial SerialAT(GSM_TXD, GSM_RXD);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);



//LED PIN AND STATUS
#define LED_PIN 13


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
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
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
  mqtt.publish(PubTopic, "OK");
  mqtt.subscribe(SubTopic);
  return mqtt.connected();
}

void loop() {

  if (mqtt.connected()) {
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

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.write(payload, len);
  Serial.println();

  //Convert Message from Byte to String
  String msg = "";
  for (int i=0;i<len;i++) {
      msg += (char)payload[i];
  }

  // Turn ON LED when subscribed msg = "ON", OFF when msg = "OFF"
  //Publish the current LED status 
  if (String(topic) == SubTopic) {
    if(msg = "ON")
    {
      digitalWrite(LED_PIN, HIGH);
      mqtt.publish(PubTopic,"ON");
    }
    else if(msg = "OFF")
    {
      digitalWrite(LED_PIN, LOW);
      mqtt.publish(PubTopic,"OFF");
    }
    
  }
}

