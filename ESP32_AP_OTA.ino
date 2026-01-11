#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

#define LED_PIN 2

const char* ap_ssid = "test_OTA";
const char* ap_password = "1234";

WebServer server(80);

const unsigned long OTA_TIMEOUT = 60000; // 1 minute
unsigned long bootTime;

unsigned long previousMillis = 0;
bool ledState = false;
bool otaActive = true;

// ----- HTML simple pour OTA -----
const char* uploadPage = R"rawliteral(
<!DOCTYPE html>
<html>
  <body>
    <h1>ESP32 OTA</h1>
    <form method='POST' action='/update' enctype='multipart/form-data'>
      <input type='file' name='update'>
      <input type='submit' value='Update'>
    </form>
  </body>
</html>
)rawliteral";

// ----- Fonctions OTA -----
void handleRoot() {
  server.send(200, "text/html", uploadPage);
}

void handleUpdate() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Upload commencé: %s\n", upload.filename.c_str());
    Update.begin(UPDATE_SIZE_UNKNOWN);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Update.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.println("OTA terminé, redémarrage...");
    } else {
      Serial.println("Erreur OTA!");
    }
  }
  if (upload.status == UPLOAD_FILE_END) {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "Update OK. Redémarrage...");
    delay(100);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ----- Configuration IP statique -----
  IPAddress local_IP(192,168,1,123);
  IPAddress gateway(192,168,1,1);
  IPAddress subnet(255,255,255,0);

  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("Point d'accès créé !");
  Serial.print("IP AP forcée: ");
  Serial.println(WiFi.softAPIP());

  // Serveur web OTA
  server.on("/", HTTP_GET, handleRoot);
  server.on("/update", HTTP_POST, [](){}, handleUpdate);
  server.begin();

  bootTime = millis();
  Serial.println("OTA actif pendant 1 minute");
}

void loop() {
  unsigned long now = millis();

  pinMode(LED_PIN, OUTPUT);

  // ----- MODE OTA -----
  if (otaActive) {
    server.handleClient();

    // LED rapide = OTA actif
    if (now - previousMillis >= 200) {
      previousMillis = now;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }

    // Timeout OTA
    if (now - bootTime > OTA_TIMEOUT) {
      otaActive = false;
      Serial.println("Timeout OTA -> mode normal");

    }
  }

  // ----- MODE NORMAL -----
  if (!otaActive) {
    // LED lente = fonctionnement normal (1 s)
    if (now - previousMillis >= 500) {
      previousMillis = now;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  }
}
