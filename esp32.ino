#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "DHT.h"

// Token Helper dan RTDB Helper
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Konfigurasi Wi-Fi
#define WIFI_SSID "qwertyuiop"
#define WIFI_PASSWORD "poiuytrewq"

// Konfigurasi Firebase
#define API_KEY "API_KEY"
#define DATABASE_URL "DATABASE_URL"

// Pin untuk sensor DHT11 dan Ultrasonic
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define TRIG_PIN 12
#define ECHO_PIN 14
#define RELAY_PIN 18
#define LED_PIN_1 19
#define LED_PIN_2 21

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
DHT dht(DHT_PIN, DHT_TYPE);

// Variabel untuk suhu, kelembapan, dan jarak
float temperature = 25.0;
float humidity = 50.0;
long duration, distance;
bool relayStatus;
bool led1Status;
bool led2Status;

// Variabel untuk memantau perubahan data
float lastTemperature = 25.0;
float lastHumidity = 50.0;
long lastDistance = 0;
bool lastRelayStatus = false;
bool lastLed1Status = false;
bool lastLed2Status = false;

void setupWiFiAndFirebase() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase sign up successful");
  } else {
    Serial.printf("Sign up error: %s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void setupDHT11() {
  dht.begin();
}

void readDHT11() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

void setupUltrasonic() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;
}

void setupRelayAndLEDs() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN_1, LOW);
  digitalWrite(LED_PIN_2, LOW);
}

void controlRelay(bool relayState) {
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
}

void controlLEDs(bool led1State, bool led2State) {
  digitalWrite(LED_PIN_1, led1State ? HIGH : LOW);
  digitalWrite(LED_PIN_2, led2State ? HIGH : LOW);
}

bool hasSensorDataChanged() {
  return temperature != lastTemperature || humidity != lastHumidity || distance != lastDistance || relayStatus != lastRelayStatus || led1Status != lastLed1Status || led2Status != lastLed2Status;
}

void sendDataToFirebase() {
  if (Firebase.ready() && hasSensorDataChanged()) {
    if (Firebase.RTDB.setFloat(&fbdo, "tubes3/temperature", temperature)) {
      Serial.println("Temperature updated: " + String(temperature));
      lastTemperature = temperature;
    }

    if (Firebase.RTDB.setFloat(&fbdo, "tubes3/humidity", humidity)) {
      Serial.println("Humidity updated: " + String(humidity));
      lastHumidity = humidity;
    }

    if (Firebase.RTDB.setFloat(&fbdo, "tubes3/distance", distance)) {
      Serial.println("Distance updated: " + String(distance));
      lastDistance = distance;
    }
  }
}

void readDataFromFirebase() {
  if (Firebase.RTDB.getBool(&fbdo, "tubes3/relayStatus")) {
    relayStatus = fbdo.boolData();
    if (relayStatus != lastRelayStatus) {
      controlRelay(relayStatus);
      lastRelayStatus = relayStatus;
    }
  }

  if (Firebase.RTDB.getBool(&fbdo, "tubes3/led1Status")) {
    led1Status = fbdo.boolData();
    if (led1Status != lastLed1Status) {
      controlLEDs(led1Status, led2Status);
      lastLed1Status = led1Status;
    }
  }

  if (Firebase.RTDB.getBool(&fbdo, "tubes3/led2Status")) {
    led2Status = fbdo.boolData();
    if (led2Status != lastLed2Status) {
      controlLEDs(led1Status, led2Status);
      lastLed2Status = led2Status;
    }
  }
}

void setup() {
  Serial.begin(115200);
  setupWiFiAndFirebase(); // Setup Wi-Fi dan Firebase
  setupDHT11();           // Setup DHT11
  setupUltrasonic();      // Setup Ultrasonic sensor
  setupRelayAndLEDs();    // Setup Relay dan LED
}

void loop() {
  readUltrasonic();        // Membaca data dari sensor ultrasonic
  readDHT11();             // Membaca data dari DHT11
  sendDataToFirebase();    // Kirim data jika ada perubahan
  readDataFromFirebase();  // Ambil data dari Firebase
  delay(1000);
}
