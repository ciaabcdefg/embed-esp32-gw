#include <HardwareSerial.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <MFRC522.h>
#include <SPI.h>

#define SS1 5 // Chip Select for Reader 1
#define SS2 4 // Chip Select for Reader 2

const char *ssid = "GalaxyIOT";
const char *password = "ciaabc12";

const char *server_url = "https://door-web.vercel.app/api/rfid";
const char *local_server_url = "http://192.168.106.152:3000/api/rfid";

const char *mqttServer = "87551fa9e51248908034dbe24e14f5d3.s2.eu.hivemq.cloud"; // Public broker (Replace with your own if needed)
const int mqttPort = 8883;
const char *mqttUser = "postman"; // If authentication is needed
const char *mqttPassword = "Cia123123";

bool isDoorOpen = false;
bool isLocked = false;

HardwareSerial SerialUART(2);
WiFiClientSecure espClient;
PubSubClient client(espClient);
MFRC522 rfid1(SS1, 2);
MFRC522 rfid2(SS2, 2);

// FUNCTIONS

void setupWiFi()
{
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LOW); // Turn off LED while disconnected
    delay(500);
    Serial.print(".");
  }

  Serial.println(" Connected!");
  digitalWrite(LED_BUILTIN, HIGH); // Turn on LED when connected
}

int timeToAutoCloseDoor = -1;
int defaultTimeToAutoCloseDoor = 50;
void handleDoor(bool open)
{
  if (isLocked)
  {
    return;
  }

  isDoorOpen = open;
  SerialUART.println(open ? "open" : "close");

  if (isDoorOpen)
  {
    timeToAutoCloseDoor = defaultTimeToAutoCloseDoor;
  }
}

void handleLock(bool locked)
{
  if (isDoorOpen)
  {
    return;
  }

  isLocked = locked;
  SerialUART.println(locked ? "lock" : "unlock");
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{

  String topicString = String(topic);
  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i]; // Append each character to the String
  }

  Serial.printf("Message received on topic '%s': '%s'\n", topicString, message);

  if (topicString == "lock")
  {
    if (message == "true")
    {
      handleLock(true);
    }
    else if (message == "false")
    {
      handleLock(false);
    }
  }
  else if (topicString == "door")
  {
    SerialUART.println(message == "true" ? "open" : "close");
    if (message == "true")
    {
      handleDoor(true);
      timeToAutoCloseDoor = -1;
    }
    else if (message == "false")
    {
      handleDoor(false);
      timeToAutoCloseDoor = -1;
    }
    else
    {
      Serial.printf("Unknown door command: %s\n", message);
    }
  }
}

void reconnectMQTT()
{
  while (!client.connected())
  {
    Serial.print("Connecting to MQTT...");
    espClient.setInsecure();
    if (client.connect("ESP32_Client", mqttUser, mqttPassword))
    {
      Serial.println(" Connected!");
      client.subscribe("lock");
      client.subscribe("door");
    }
    else
    {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void readRFID(MFRC522 &rfid, int csPin)
{
  digitalWrite(csPin, LOW);
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
  {
    digitalWrite(csPin, HIGH);
    return;
  }

  Serial.print("Reader ");
  Serial.print(csPin == SS1 ? "1" : "2");
  Serial.print(" - Card UID: ");

  int uid;

  for (byte i = 0; i < rfid.uid.size; i++)
  {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfid.uid.uidByte[i], HEX);
    uid = (uid << 8) | rfid.uid.uidByte[i];
  }
  Serial.println();
  Serial.println(uid, HEX);

  handleDoor(!isDoorOpen);

  if (isDoorOpen)
  {
    handleDoor(true);
    timeToAutoCloseDoor = defaultTimeToAutoCloseDoor;
  }
  else
  {
    handleDoor(true);
  }

  // HTTPUtils::post(String(local_server_url), String(uid, HEX));

  rfid.PICC_HaltA();
  digitalWrite(csPin, HIGH);
}

void autoCloseLoop()
{
  if (timeToAutoCloseDoor > 0)
  {
    timeToAutoCloseDoor -= 1;
    Serial.printf("Time remaining: %d\n", timeToAutoCloseDoor);
  }
  else if (timeToAutoCloseDoor == 0)
  {
    handleDoor(false);
    timeToAutoCloseDoor = -1;
  }
}

// MAIN FUNCTIONS

void setup()
{
  Serial.begin(9600);
  SerialUART.begin(921600, SERIAL_8N1, 16, 17);

  handleDoor(false);
  handleLock(false);

  SPI.begin();

  pinMode(LED_BUILTIN, OUTPUT); // Set LED as output
  pinMode(SS1, OUTPUT);
  pinMode(SS2, OUTPUT);
  digitalWrite(SS1, HIGH);
  digitalWrite(SS2, HIGH);

  rfid1.PCD_Init();
  rfid2.PCD_Init();
  Serial.println("MFRC522 initiated");

  setupWiFi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi lost. Reconnecting...");
    digitalWrite(LED_BUILTIN, LOW); // Turn off LED while disconnected
    setupWiFi();
  }

  if (!client.connected())
  {
    reconnectMQTT();
  }

  readRFID(rfid1, SS1);
  readRFID(rfid2, SS2);

  autoCloseLoop();
  client.loop();
}
