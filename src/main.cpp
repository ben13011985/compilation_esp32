#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "secrets.h" // créer src/secrets.h à partir de src/secrets.example.h

const int LED_PIN = 2; // LED intégrée sur beaucoup de cartes dev

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  delay(100);
  Serial.println("OTA_ESP32_BM - démarrage");

  // Connexion WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connexion WiFi...");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connecté, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Échec connexion WiFi (continuer en local)");
  }

  // Configuration ArduinoOTA
  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA end");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready");
}

void loop() {
  ArduinoOTA.handle();

  // Blink simple
  static unsigned long last = 0;
  if (millis() - last >= 500) {
    last = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}