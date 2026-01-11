#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include "driver/ledc.h"

// ================= WIFI AP =================
const char* ssid = "test_bm";
const char* password = "1234";

IPAddress local_IP(192,168,10,10);
IPAddress gateway(192,168,10,10);
IPAddress subnet(255,255,255,0);

// ================= PWM =================
#define PWM_PIN       18
#define PWM_FREQ      1000
#define PWM_RES       LEDC_TIMER_10_BIT
#define PWM_CHANNEL   LEDC_CHANNEL_0
#define PWM_TIMER     LEDC_TIMER_0
#define PWM_MODE      LEDC_LOW_SPEED_MODE

WebServer server(80);

// ================= PWM FUNCTION =================
void setPWM(uint32_t duty)
{
  ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty);
  ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}

// ================= HTML PAGE =================
String pageHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>EV Control</title>
<style>
body { background:#111; color:white; font-family:Arial; margin:0; }
h1 { text-align:center; margin:20px; }
button { width:90%; height:90px; font-size:32px; margin:15px auto; display:block; border-radius:20px; border:none; background:#555; color:#888; }
.inactive { background:#555; color:#888; }
.active-on { background:#00a050; color:white; }
.active-off { background:#c00020; color:white; }
</style>
</head>
<body>

<h1>Courant de charge</h1>

<button id="off"  onclick="activate('off','/off')">OFFfff</button>
<button id="a6"   onclick="activate('a6','/6a')">6 A</button>
<button id="a8"   onclick="activate('a8','/8a')">8 A</button>
<button id="a10"  onclick="activate('a10','/10a')">10 A</button>
<button id="a12"  onclick="activate('a12','/12a')">12 A</button>

<button onclick="window.location='/ota'">Reserved (OTA)</button>

<script>
const ids = ['off','a6','a8','a10','a12'];

function resetAll() { ids.forEach(id => { document.getElementById(id).className = 'inactive'; }); }

function activate(id, url) {
  resetAll();
  if (id === 'off') document.getElementById(id).className = 'active-off';
  else document.getElementById(id).className = 'active-on';
  fetch(url);
}

window.onload = () => {
  resetAll();
  document.getElementById('off').className = 'active-off';
};
</script>

</body>
</html>
)rawliteral";
}

// ================= OTA PAGE =================
String otaHTML() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>OTA Update</title>
</head>
<body style="background:#111;color:white;font-family:Arial;text-align:center;">
<h2>OTA Upload</h2>
<form method="POST" action="/update" enctype="multipart/form-data">
  <input type="file" name="update">
  <input type="submit" value="Upload">
</form>
<p id="timer">Retour automatique dans 30 secondes...</p>
<script>
let countdown = 30;
let timer = setInterval(() => {
  countdown--;
  document.getElementById('timer').innerText = 'Retour automatique dans ' + countdown + ' secondes...';
  if(countdown <= 0) window.location = '/';
}, 1000);
</script>
</body>
</html>
)rawliteral";
}

// ================= ROUTES =================
void setupRoutes() {

  server.on("/", [](){
    server.send(200, "text/html", pageHTML());
  });

  server.on("/off", [](){ setPWM(0); server.send(200, "text/plain", "OFF"); });
  server.on("/6a", [](){ setPWM(102); server.send(200, "text/plain", "6A"); });
  server.on("/8a", [](){ setPWM(136); server.send(200, "text/plain", "8A"); });
  server.on("/10a", [](){ setPWM(171); server.send(200, "text/plain", "10A"); });
  server.on("/12a", [](){ setPWM(205); server.send(200, "text/plain", "12A"); });

  // OTA page
  server.on("/ota", [](){
    server.send(200, "text/html", otaHTML());
  });

  // OTA upload handler
  server.on("/update", HTTP_POST, [](){
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
    ESP.restart();
  }, [](){
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
      Serial.printf("Update Start: %s\n", upload.filename.c_str());
      if(!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
    } else if(upload.status == UPLOAD_FILE_WRITE){
      if(Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
    } else if(upload.status == UPLOAD_FILE_END){
      if(Update.end(true)) Serial.printf("Update Success: %u bytes\n", upload.totalSize);
      else Update.printError(Serial);
    }
  });
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  // PWM TIMER
  ledc_timer_config_t timer;
  timer.speed_mode = PWM_MODE;
  timer.timer_num = PWM_TIMER;
  timer.duty_resolution = PWM_RES;
  timer.freq_hz = PWM_FREQ;
  timer.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&timer);

  // PWM CHANNEL
  ledc_channel_config_t channel;
  channel.gpio_num = PWM_PIN;
  channel.speed_mode = PWM_MODE;
  channel.channel = PWM_CHANNEL;
  channel.timer_sel = PWM_TIMER;
  channel.duty = 0;
  channel.hpoint = 0;
  ledc_channel_config(&channel);

  // WIFI AP
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP démarré");
  Serial.print("IP : "); Serial.println(WiFi.softAPIP());

  setupRoutes();
  server.begin();
}

// ================= LOOP =================
void loop() {
  server.handleClient();
}
