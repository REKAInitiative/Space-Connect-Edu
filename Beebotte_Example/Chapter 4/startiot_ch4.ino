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


#define RIG_RXD 2
#define RIG_TXD 4
#define RIG_STATUS 6
#define RIG_PWRKEY 7


#include <SoftwareSerial.h>
SoftwareSerial SerialAT(RIG_TXD, RIG_RXD); // RX, TX


const char apn[]  = "";
const char user[] = "";
const char pass[] = "";

// Name of the server we want to connect to
const char server[] = "www.startiot.co";
const int  port     = 80;
// Path to download (this is the bit after the hostname in the URL)
const char resource[] = "/api/get.php?id=my123";

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient http(client, server, port);

void(* resetFunc) (void) = 0;

void setup() {
  RIG_INIT(RIG_STATUS, RIG_PWRKEY);
  if(!RIG_ON())
  {
    Serial.print( F("\n\nGSM FAIL TURN ON\n") );
    delay(100);
    resetFunc();
  }
  // Set console baud rate
  Serial.begin(115200);
  delay(10);

  // Set GSM module baud rate
  SerialAT.begin(9600);
  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  Serial.println("Initializing modem...");
  modem.restart();


  // Unlock your SIM card with a PIN
  //modem.simUnlock("1234");
}

void loop() {
  Serial.print(F("Waiting for network..."));
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" OK");

  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    delay(10000);
    return;
  }
  Serial.println(" OK");


  Serial.print(F("Performing HTTP GET request... "));
  int err = http.get(resource);
  if (err != 0) {
    Serial.println("failed to connect");
    delay(10000);
    return;
  }

  int status = http.responseStatusCode();
  Serial.println(status);
  if (!status) {
    delay(10000);
    return;
  }

  while (http.headerAvailable()) {
    String headerName = http.readHeaderName();
    String headerValue = http.readHeaderValue();
    //Serial.println(headerName + " : " + headerValue);
  }

  int length = http.contentLength();
  if (length >= 0) {
    Serial.println(String("Content length is: ") + length);
  }
  if (http.isResponseChunked()) {
    Serial.println("This response is chunked");
  }

  String body = http.responseBody();
  Serial.println("Response:");
  
  Serial.println(body);
  
  Serial.println(String("Body length is: ") + body.length());

  // Shutdown

  http.stop();

  modem.gprsDisconnect();
  Serial.println("GPRS disconnected");

  // Do nothing forevermore
  while (true) {
    delay(1000);
  }
}

void RIG_INIT(int stat,int pwrkey)
{
  pinMode(RIG_STATUS, INPUT);
  pinMode(RIG_PWRKEY, OUTPUT);
}

bool RIG_ON()
{
  long masa = 0;
  if(!digitalRead(RIG_STATUS))
  {
    masa = 5000 + millis();
    digitalWrite(RIG_PWRKEY, HIGH);
    while(1)
    {
      if(digitalRead(RIG_STATUS))
      {
        digitalWrite(RIG_PWRKEY, LOW);
        return 1;
      }
      else if( masa < millis() )
      {
        digitalWrite(RIG_PWRKEY, LOW);
        return 0;
      }
    }
  }
  return 1;
}

bool RIG_STATUS_POWER()
{
  static long masa = 0;
  if( digitalRead(RIG_STATUS) )
  {
    masa = 3000 + millis();
  }
  else if( masa < millis() )
  {
    return 0;
  }
  return 1;
}

