#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include "driver/ledc.h"

#include "lecture_puissance.h"

// ================= WIFI AP =================
const char* ssid = "test_bm";
const char* password = "1234";

IPAddress local_IP(192,168,10,10);
IPAddress gateway(192,168,10,10);
IPAddress subnet(255,255,255,0);

WebServer server(80);

// ================= PWM =================
#define PWM_PIN       18
#define PWM_FREQ      1000
#define PWM_RES       LEDC_TIMER_10_BIT
#define PWM_CHANNEL   LEDC_CHANNEL_0
#define PWM_TIMER     LEDC_TIMER_0
#define PWM_MODE      LEDC_LOW_SPEED_MODE

// ================= COURANT/PUISSANCE =================
// ⚠️ GPIO13 = ADC2 => peut être instable avec WiFi selon cartes/SDK.
// Tu as demandé à rester en GPIO13 : on applique.
#define PIN_MESURE    13
#define RATIO_AZCT02  100.0f     // A / V RMS (à ajuster)
#define VRMS_GRID     230.0f

LecturePuissance mesure(PIN_MESURE, RATIO_AZCT02, VRMS_GRID);

volatile float g_IrmsA = 0.0f;
volatile float g_PkW   = 0.0f;

enum Mode : uint8_t { MODE_OFF=0, MODE_6A, MODE_8A, MODE_10A, MODE_12A, MODE_AUTO };
volatile Mode g_mode = MODE_OFF;
volatile int  g_setA = 0;

// PWM duty values (10-bit)
static constexpr uint32_t DUTY_OFF = 0;
static constexpr uint32_t DUTY_6A  = 102; // ~10%
static constexpr uint32_t DUTY_8A  = 136; // ~13.3%
static constexpr uint32_t DUTY_10A = 171; // ~16.7%
static constexpr uint32_t DUTY_12A = 205; // ~20%

// ================= PWM FUNCTION =================
void setPWM(uint32_t duty)
{
  ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty);
  ledc_update_duty(PWM_MODE, PWM_CHANNEL);
}

void applyManualSetpoint(int amps)
{
  g_setA = amps;
  switch(amps) {
    case 6:  setPWM(DUTY_6A);  g_mode = MODE_6A;  break;
    case 8:  setPWM(DUTY_8A);  g_mode = MODE_8A;  break;
    case 10: setPWM(DUTY_10A); g_mode = MODE_10A; break;
    case 12: setPWM(DUTY_12A); g_mode = MODE_12A; break;
    default: setPWM(DUTY_OFF); g_mode = MODE_OFF; g_setA = 0; break;
  }
}

void applyOff()
{
  g_mode = MODE_OFF;
  g_setA = 0;
  setPWM(DUTY_OFF);
}

void applyAuto()
{
  g_mode = MODE_AUTO;
  // on garde g_setA tel quel, l'algo va l'ajuster
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
.active-auto { background:#00a050; color:white; }

#statusBox {
  width:90%;
  height:90px;
  font-size:22px;
  margin:15px auto;
  display:flex;
  flex-direction:column;
  align-items:center;
  justify-content:center;
  border-radius:20px;
  background:#222;
  color:white;
  border: 2px solid #444;
}
#statusLine1 { font-size:26px; font-weight:bold; }
#statusLine2 { font-size:18px; color:#aaa; margin-top:6px; }
</style>
</head>
<body>

<h1>Courant de charge</h1>

<div id="statusBox">
  <div id="statusLine1">Power : <span id="pkw">--.--</span> kW</div>
  <div id="statusLine2">Set current : <span id="seta">--</span> A</div>
</div>

<button id="off"   onclick="activate('off','/off')">OFF</button>
<button id="auto"  onclick="activate('auto','/auto')">AUTO</button>
<button id="a6"    onclick="activate('a6','/6a')">6 A</button>
<button id="a8"    onclick="activate('a8','/8a')">8 A</button>
<button id="a10"   onclick="activate('a10','/10a')">10 A</button>
<button id="a12"   onclick="activate('a12','/12a')">12 A</button>

<button onclick="window.location='/ota'">Reserved (OTA)</button>

<script>
const ids = ['off','auto','a6','a8','a10','a12'];

function resetAll() { ids.forEach(id => { document.getElementById(id).className = 'inactive'; }); }

function activate(id, url) {
  resetAll();
  if (id === 'off') document.getElementById(id).className = 'active-off';
  else if (id === 'auto') document.getElementById(id).className = 'active-auto';
  else document.getElementById(id).className = 'active-on';
  fetch(url);
}

async function refreshStatus(){
  try{
    const r = await fetch('/status');
    const j = await r.json();
    document.getElementById('pkw').innerText  = j.pkw.toFixed(2);
        document.getElementById('seta').innerText = j.setA;

    // refléter le mode côté UI
    resetAll();
    if (j.mode === 'OFF') document.getElementById('off').className = 'active-off';
    else if (j.mode === 'AUTO') document.getElementById('auto').className = 'active-auto';
    else if (j.mode === '6A') document.getElementById('a6').className = 'active-on';
    else if (j.mode === '8A') document.getElementById('a8').className = 'active-on';
    else if (j.mode === '10A') document.getElementById('a10').className = 'active-on';
    else if (j.mode === '12A') document.getElementById('a12').className = 'active-on';
  }catch(e){
    // ignore
  }
}

window.onload = () => {
  refreshStatus();
  setInterval(refreshStatus, 10000); // toutes les 10 s
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

// ================= TASK MESURE (1ms, non-bloquante) =================
void mesureTask(void* pv) {
  // Atténuation large (si ton signal peut monter)
  analogSetAttenuation(ADC_11db);

  Serial.println("Calibration puissance/courant (10s)...");
  mesure.startCalibration(10000);
  g_IrmsA = 0.0f;
  g_PkW   = 0.0f;

  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(1);

  while(true) {
    vTaskDelayUntil(&lastWake, period);

    const bool newVal = mesure.sample();
    if (mesure.isCalibrated() && newVal) {
      g_IrmsA = mesure.getCurrentRMS_A();
      g_PkW   = mesure.getPower_kW();
      Serial.print("P = "); Serial.print((float)g_PkW, 3);
      Serial.print(" kW | Irms = "); Serial.print((float)g_IrmsA, 2);
      Serial.print(" A | offsetLSB="); Serial.println(mesure.getOffsetLSB(), 1);
    }
  }
}

// ================= TASK AUTO (décision toutes les 10s après update mesure) =================
static float predictPkW(int amps) {
  return (VRMS_GRID * (float)amps) / 1000.0f;
}

static void setByAmps(int amps) {
  switch(amps) {
    case 6:  setPWM(DUTY_6A);  break;
    case 8:  setPWM(DUTY_8A);  break;
    case 10: setPWM(DUTY_10A); break;
    case 12: setPWM(DUTY_12A); break;
    default: setPWM(DUTY_OFF); break;
  }
  g_setA = amps;
}

void autoTask(void* pv) {
  uint32_t lastEval = 0;
  while(true) {
    vTaskDelay(pdMS_TO_TICKS(200)); // petite boucle

    if (g_mode != MODE_AUTO) continue;

    // on évalue à la fréquence des mesures (toutes les ~10s)
    uint32_t now = millis();
    if (now - lastEval < 9500) continue;
    lastEval = now;

    // Condition utilisateur: rester < 18 kW
    const float limit_kW = 18.0f;
    const float p_meas = (float)g_PkW;

    // Stratégie:
    // - Si puissance mesurée > limite : on descend d'un cran
    // - Sinon : on monte au max possible (12->10->8->6) sous limite (avec prédiction simple)
    int target = g_setA;
    if (target == 0) target = 6;

    if (p_meas > limit_kW) {
      if (target > 10) target = 10;
      else if (target > 8) target = 8;
      else if (target > 6) target = 6;
      else target = 0; // even 6A is too much -> pause charge but stay in AUTO
    } else {
      // essai du max
      const int candidates[4] = {12,10,8,6};
      target = 6;
      for (int a : candidates) {
        if (predictPkW(a) <= limit_kW) { target = a; break; }
      }
    }

    setByAmps(target);
  }
}

// ================= ROUTES =================
const char* modeToStr(Mode m) {
  switch(m) {
    case MODE_OFF: return "OFF";
    case MODE_AUTO: return "AUTO";
    case MODE_6A: return "6A";
    case MODE_8A: return "8A";
    case MODE_10A: return "10A";
    case MODE_12A: return "12A";
    default: return "OFF";
  }
}

void setupRoutes() {

  server.on("/", [](){
    server.send(200, "text/html", pageHTML());
  });

  server.on("/off", [](){ applyOff(); server.send(200, "text/plain", "OFF"); });

  server.on("/auto", [](){ applyAuto(); server.send(200, "text/plain", "AUTO"); });

  server.on("/6a",  [](){ applyManualSetpoint(6);  server.send(200, "text/plain", "6A"); });
  server.on("/8a",  [](){ applyManualSetpoint(8);  server.send(200, "text/plain", "8A"); });
  server.on("/10a", [](){ applyManualSetpoint(10); server.send(200, "text/plain", "10A"); });
  server.on("/12a", [](){ applyManualSetpoint(12); server.send(200, "text/plain", "12A"); });

  // status JSON
  server.on("/status", [](){
    String s = "{";
    s += "\"pkw\":" + String((float)g_PkW, 4) + ",";
    s += "\"irms\":" + String((float)g_IrmsA, 4) + ",";
    s += "\"setA\":" + String((int)g_setA) + ",";
    s += "\"mode\":\"" + String(modeToStr((Mode)g_mode)) + "\"";
    s += "}";
    server.send(200, "application/json", s);
  });

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

  // PWM TIMER (zero-init)
  ledc_timer_config_t timer = {};
  timer.speed_mode = PWM_MODE;
  timer.timer_num = PWM_TIMER;
  timer.duty_resolution = PWM_RES;
  timer.freq_hz = PWM_FREQ;
  timer.clk_cfg = LEDC_AUTO_CLK;
  ledc_timer_config(&timer);

  // PWM CHANNEL (zero-init)
  ledc_channel_config_t channel = {};
  channel.gpio_num = PWM_PIN;
  channel.speed_mode = PWM_MODE;
  channel.channel = PWM_CHANNEL;
  channel.timer_sel = PWM_TIMER;
  channel.duty = 0;
  channel.hpoint = 0;
  ledc_channel_config(&channel);

  // Default state
  applyOff();

  // WIFI AP
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP démarré");
  Serial.print("IP : "); Serial.println(WiFi.softAPIP());

  // Init mesure
  mesure.begin(1000);
  mesure.setRmsWindow(10000);

  setupRoutes();
  server.begin();

  // Mesure sur CPU1 (comme avant pour éviter WDT)
  xTaskCreatePinnedToCore(mesureTask, "mesureTask", 4096, nullptr, 1, nullptr, 1);
  // Auto sur CPU0 (léger)
  xTaskCreatePinnedToCore(autoTask, "autoTask", 4096, nullptr, 1, nullptr, 0);
}

// ================= LOOP =================
void loop() {
  server.handleClient();
}
