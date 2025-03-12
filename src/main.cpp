#include <ArduinoJson.h>
#include <HTTPUtils.h>
#include <HardwareSerial.h>
#include <MFRC522.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <unordered_set>

#define SS1 5  // Chip Select for Reader 1
#define SS2 4  // Chip Select for Reader 2

const char *ssid = "GalaxyIOT";
const char *password = "12345678";

#define USE_LOCAL 1

#if USE_LOCAL
const char *server_url = "http://192.168.106.152:3000/api";
#else
const char *server_url = "https://door-web.vercel.app/api";
#endif

const char *mqttServer =
    "87551fa9e51248908034dbe24e14f5d3.s2.eu.hivemq.cloud";

const int mqttPort = 8883;
const char *mqttUser = "postman";  // If authentication is needed
const char *mqttPassword = "Cia123123";

bool isDoorOpen = false;
bool isLocked = false;

HardwareSerial SerialUART(2);
WiFiClientSecure espClient;
PubSubClient client(espClient);
MFRC522 rfid1(SS1, 2);
MFRC522 rfid2(SS2, 2);

std::unordered_set<unsigned int> petUIDs;

const char *mqttTopics[] = {
    "door", "lock", "status", "door/status/req", "lock/status/req", "pet"};

void updatePetList();

// ---------- WIFI ---------- //

void setupWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println(" Connected!");
}

// ---------- HANDLERS ---------- //

int timeToAutoCloseDoor = -1;
int defaultTimeToAutoCloseDoor = 50;

void handleDoor(bool open) {
    if (isLocked) {
        return;
    }

    isDoorOpen = open;
    SerialUART.println(open ? "open" : "close");
    client.publish("door/status/res", isDoorOpen ? "true" : "false");

    if (isDoorOpen) {
        timeToAutoCloseDoor = defaultTimeToAutoCloseDoor;
    }
}

void handleLock(bool locked) {
    if (isDoorOpen) {
        return;
    }

    isLocked = locked;
    SerialUART.println(locked ? "lock" : "unlock");
    client.publish("lock/status/res", isLocked ? "true" : "false");
}

void handleAutoCloseLoop() {
    if (timeToAutoCloseDoor > 0) {
        timeToAutoCloseDoor -= 1;
        Serial.printf("Time remaining: %d\n", timeToAutoCloseDoor);
    } else if (timeToAutoCloseDoor == 0) {
        handleDoor(false);
        timeToAutoCloseDoor = -1;
    }
}

// ---------- MQTT ---  ------- //

void connectMQTT() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        espClient.setInsecure();
        if (client.connect("ESP32_Client", mqttUser, mqttPassword)) {
            Serial.println(" Connected!");

            for (auto topic : mqttTopics) {
                client.subscribe(topic);
                Serial.print("Subscribed to topic: ");
                Serial.println(topic);
            }
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Retrying in 5 seconds...");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    String topicString = String(topic);
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];  // Append each character to the String
    }

    Serial.printf("Topic '%s': '%s'\n", topicString, message);

    if (topicString == "lock") {
        if (message == "true") {
            handleLock(true);
        } else if (message == "false") {
            handleLock(false);
        }
    } else if (topicString == "door") {
        SerialUART.println(message == "true" ? "open" : "close");
        if (message == "true") {
            handleDoor(true);
            timeToAutoCloseDoor = -1;
        } else if (message == "false") {
            handleDoor(false);
            timeToAutoCloseDoor = -1;
        } else {
            Serial.printf("Unknown door command: %s\n", message);
        }
    } else if (topicString == "status") {
        if (message == "ping") {
            client.publish("status", "online");
        }
    } else if (topicString == "door/status/req") {
        if (message == "ping") {
            client.publish("door/status/res", isDoorOpen ? "true" : "false");
        }
    } else if (topicString == "lock/status/req") {
        if (message == "ping") {
            client.publish("lock/status/res", isLocked ? "true" : "false");
        }
    } else if (topicString == "pet") {
        if (message == "changed") {
            updatePetList();
        }
    }
}

void updatePetList() {
    Serial.println("Updating pet list...");
    String endpoint = String(server_url) + String("/pet?uids=true");
    String payload = HTTPUtils::get(endpoint);

    petUIDs = std::unordered_set<unsigned int>();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return;
    }

    JsonArray array = doc.as<JsonArray>();

    for (JsonVariant v : array) {
        String uidString = v.as<String>();
        unsigned int uid = strtoul(uidString.c_str(), nullptr, 16);
        petUIDs.insert(uid);
    }

    Serial.print("Updated pet list: ");
    for (auto uid : petUIDs) {
        Serial.print(String(uid, HEX));
        Serial.print(", ");
    }
    Serial.println();
}

void readRFID(MFRC522 &rfid, int csPin) {
    digitalWrite(csPin, LOW);
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        digitalWrite(csPin, HIGH);
        return;
    }

    Serial.print("Reader ");
    Serial.print(csPin == SS1 ? "1" : "2");
    Serial.print(" - Card UID: ");

    unsigned long uid;

    for (byte i = 0; i < rfid.uid.size; i++) {
        Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(rfid.uid.uidByte[i], HEX);
        uid = (uid << 8) | rfid.uid.uidByte[i];
    }
    Serial.println();
    Serial.println(uid, HEX);

    JsonDocument payload;
    int prevTime = timeToAutoCloseDoor;

    if (petUIDs.find(uid) != petUIDs.end()) {
        handleDoor(true);
        payload["allowed"] = true;
    } else {
        payload["allowed"] = false;
    }

    if ((isDoorOpen && prevTime == -1) || !isDoorOpen) {
        JsonDocument payload;
        payload["uid_str"] = String(uid, HEX);
        payload["rfid"] = csPin == SS1 ? 0 : 1;
        payload["opened"] = isDoorOpen;

        String jsonString;
        serializeJson(payload, jsonString);

        HTTPUtils::postAsync(String(server_url) + String("/door/log"), jsonString);
    }

    payload["rfid"] = String(uid, HEX);
    String jsonString;
    serializeJson(payload, jsonString);
    client.publish("rfid", jsonString.c_str());

    // HTTPUtils::post(String(local_server_url), String(uid, HEX));

    rfid.PICC_HaltA();
    digitalWrite(csPin, HIGH);
}

// ---------- MAIN FUNCTIONS ---------- //

void setup() {
    Serial.begin(9600);
    SerialUART.begin(9600, SERIAL_8N1, 16, 17);

    handleDoor(false);
    handleLock(false);

    SPI.begin();

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

    updatePetList();
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost. Reconnecting...");
        setupWiFi();
    }

    if (!client.connected()) {
        connectMQTT();
    }

    readRFID(rfid1, SS1);
    readRFID(rfid2, SS2);

    handleAutoCloseLoop();
    client.loop();
}
