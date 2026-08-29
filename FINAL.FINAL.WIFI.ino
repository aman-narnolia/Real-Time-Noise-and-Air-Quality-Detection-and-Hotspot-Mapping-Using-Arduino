/*
  ESP8266 Environmental Monitor Dashboard
  Arduino Pins 2,3 → ESP8266 Serial (9600 baud)
  Format: mq135_ppm,mq7_raw,sound_db\n
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "ThingSpeak.h"

// === YOUR SETTINGS ===
const char* WIFI_SSID = "AMANS-LAPTOP";     // Change to your WiFi
const char* WIFI_PASS = "12121212";           // Change to your password

unsigned long myChannelNumber = 3166606;       // ThingSpeak Channel ID
const char* myWriteAPIKey = "60HM2KF7T72F04F8"; // ThingSpeak .Write Key

// Hardware (optional buzzer control)
const uint8_t BUZZER_PIN = D5;  // GPIO4

WiFiClient client;
ESP8266WebServer server(80);

// Global sensor data
float mq135_ppm = 0.0;
int mq7_raw = 0;
int sound_db = 0;
bool alertActive = false;
bool thingSpeakEnabled = true;
String serialBuffer = "";
unsigned long lastThingSpeakUpdate = 0;
const unsigned long THINGSPEAK_INTERVAL = 20000UL; // 20s

void setup() {
  Serial.begin(9600);  // MUST match Arduino SoftwareSerial 9600
  delay(200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  Serial.println("\n=== ESP8266 + Arduino Monitor Starting ===");
  Serial.println("Waiting for Arduino data on Pins 2,3...");
  
  // Connect WiFi
  connectWiFi();
  
  // ThingSpeak
  ThingSpeak.begin(client);
  
  // Web routes
  server.on("/", handleRoot);
  server.on("/data", handleDataJSON);
  server.on("/cmd", handleCmd);
  server.begin();
  
  Serial.println("Web server ready!");
  Serial.print("Dashboard: http://");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(WiFi.localIP());
  }
}

void loop() {
  server.handleClient();
  
  // Read Arduino CSV data from Serial (Pins 2,3 connection)
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        Serial.print("Arduino→ESP: ");
        Serial.println(serialBuffer);
        parseArduinoCSV(serialBuffer);
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
    }
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  Serial.print("Connecting WiFi");
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 20) {
    delay(500);
    Serial.print(".");
    i++;
  }
  
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("✅ WiFi OK! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("❌ WiFi failed - AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("EnvMonitor", "12345678");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }
}

void parseArduinoCSV(String line) {
  line.trim();
  if (line.length() == 0) return;
  
  // Parse: "123.4,56,78"
  int c1 = line.indexOf(',');
  int c2 = line.indexOf(',', c1 + 1);
  
  if (c1 > 0 && c2 > c1) {
    mq135_ppm = line.substring(0, c1).toFloat();
    mq7_raw = line.substring(c1 + 1, c2).toInt();
    sound_db = line.substring(c2 + 1).toInt();
    
    // Safety clamp
    mq135_ppm = constrain(mq135_ppm, 0, 5000);
    mq7_raw = constrain(mq7_raw, 0, 1023);
    sound_db = constrain(sound_db, 0, 150);
    
    Serial.printf("Parsed → MQ135:%.1f MQ7:%d Sound:%d\n", mq135_ppm, mq7_raw, sound_db);
    
    updateAlertStatus();
    uploadThingSpeak();
  }
}

void updateAlertStatus() {
  // Match your Arduino thresholds
  int airSeverity = 0;
  if (mq135_ppm > 1200) airSeverity = 1;
  if (mq135_ppm > 2000) airSeverity = 2;
  if (mq135_ppm > 4000) airSeverity = 3;
  
  int coSeverity = 0;
  if (mq7_raw > 100) coSeverity = 1;
  if (mq7_raw > 199) coSeverity = 2;
  if (mq7_raw > 300) coSeverity = 3;
  
  alertActive = (airSeverity >= 2 || coSeverity >= 2 || sound_db > 80);
  
  Serial.printf("Alert: %s (Air=%d, CO=%d)\n", 
                alertActive ? "🚨 ACTIVE" : "✅ OK", airSeverity, coSeverity);
}

void uploadThingSpeak() {
  if (!thingSpeakEnabled || WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastThingSpeakUpdate < THINGSPEAK_INTERVAL) return;
  
  ThingSpeak.setField(1, mq135_ppm);
  ThingSpeak.setField(2, mq7_raw);
  ThingSpeak.setField(3, sound_db);
  ThingSpeak.setField(4, alertActive ? 1 : 0);
  
  int status = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  Serial.printf("ThingSpeak: %s\n", status == 200 ? "OK" : "Failed");
  lastThingSpeakUpdate = millis();
}

// === WEB DASHBOARD ===
void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>🌡️ Env Monitor Dashboard</title>
  <style>
    *{margin:0;padding:0;box-sizing:border-box}
    body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;
         background:linear-gradient(135deg,#1e3c72,#2a5298);color:#fff;
         min-height:100vh;padding:20px;max-width:900px;margin:auto}
    h1{font-size:2.2em;text-align:center;margin:20px 0 30px;
       background:linear-gradient(45deg,#ff6b6b,#4ecdc4); 
       -webkit-background-clip:text;-webkit-text-fill-color:transparent;
       background-clip:text}
    .metrics{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));
             gap:20px;margin:30px 0}
    .metric{background:hsla(0,0%,100%,.1);backdrop-filter:blur(10px);
           border-radius:20px;padding:25px;border:1px solid hsla(0,0%,100%,.2);
           transition:all .3s ease}
    .metric:hover{transform:translateY(-5px);box-shadow:0 20px 40px rgba(0,0,0,.3)}
    .label{font-size:.85em;color:hsla(0,0%,100%,.7);text-transform:uppercase;
           letter-spacing:1px;margin-bottom:10px}
    .value{font-size:3em;font-weight:700;line-height:1;
           background:linear-gradient(135deg,#fff,#f0f0f0); 
           -webkit-background-clip:text;-webkit-text-fill-color:transparent;
           background-clip:text;display:block}
    .alert .value,.alert .label{color:#ff6b6b !important;
                               text-shadow:0 0 20px rgba(255,107,107,.5) !important}
    .controls{display:flex;flex-wrap:wrap;gap:15px;justify-content:center;
              margin:40px 0;padding:25px;background:hsla(0,0%,100%,.1);
              border-radius:20px;border:1px solid hsla(0,0%,100%,.2)}
    .btn{padding:15px 30px;border:none;border-radius:50px;font-size:1.1em;
         font-weight:600;cursor:pointer;transition:all .3s ease;
         background:linear-gradient(135deg,#667eea,#764ba2);color:white;
         box-shadow:0 10px 30px rgba(102,126,234,.4)}
    .btn:hover{transform:translateY(-2px);box-shadow:0 15px 40px rgba(102,126,234,.6)}
    .btn-secondary{background:linear-gradient(135deg,#f093fb,#f5576c)}
    .status-bar{padding:20px;background:hsla(0,0%,100%,.1);border-radius:15px;
                margin-top:30px;border-left:5px solid #4ecdc4}
    #log{max-height:200px;overflow-y:auto;background:hsla(0,0%,0%,.3);
         border-radius:10px;padding:15px;font-family:monospace;font-size:.85em;
         line-height:1.4}
  </style>
</head>
<body>
  <h1>🌡️ Environmental Monitor</h1>
  
  <div class="metrics" id="metrics">
    <div class="metric" id="mq135">
      <div class="label">Air Quality MQ-135</div>
      <div class="value">--</div>
      <div class="unit">PPM</div>
    </div>
    <div class="metric" id="mq7">
      <div class="label">CO MQ-7</div>
      <div class="value">--</div>
      <div class="unit">Raw</div>
    </div>
    <div class="metric" id="sound">
      <div class="label">Sound Level</div>
      <div class="value">--</div>
      <div class="unit">dB</div>
    </div>
  </div>
  
  <div class="controls">
    <button class="btn" onclick="toggleBuzzer(1)">🔊 Buzzer ON</button>
    <button class="btn btn-secondary" onclick="toggleBuzzer(0)">🔇 Buzzer OFF</button>
    <button class="btn" onclick="refreshNow()">🔄 Refresh Now</button>
    <button class="btn" onclick="testAlert()">🚨 Test Alert</button>
  </div>
  
  <div class="status-bar">
    <div>📶 WiFi: <span id="wifiStatus">Connecting...</span></div>
    <div>⏰ Last Update: <span id="lastUpdate">--</span></div>
    <div>🚨 Status: <span id="alertStatus">Checking...</span></div>
    <div>🌐 IP: <span id="localIP">--</span></div>
    <div style="margin-top:15px">
      <div>📋 Recent Activity:</div>
      <div id="log"></div>
    </div>
  </div>

  <script>
    let lastData = {};
    
    function log(msg) {
      const logDiv = document.getElementById('log');
      const time = new Date().toLocaleTimeString();
      logDiv.innerHTML = `[${time}] ${msg}<br>` + logDiv.innerHTML.slice(0, 1000);
    }
    
    async function updateDisplay(data) {
      lastData = data;
      
      document.getElementById('mq135').querySelector('.value').textContent = 
        data.mq135_ppm?.toFixed(1) ?? '--';
      document.getElementById('mq7').querySelector('.value').textContent = 
        data.mq7_raw ?? '--';
      document.getElementById('sound').querySelector('.value').textContent = 
        data.sound_db ?? '--';
      
      const now = new Date().toLocaleTimeString();
      document.getElementById('lastUpdate').textContent = now;
      
      // Alert styling
      const metrics = document.getElementById('metrics');
      if (data.alertActive) {
        metrics.classList.add('alert');
        document.getElementById('alertStatus').innerHTML = 
          '<span style="color:#ff6b6b;font-weight:bold">🚨 ALERT ACTIVE</span>';
        log('🚨 ALERT: High pollution or noise detected');
      } else {
        metrics.classList.remove('alert');
        document.getElementById('alertStatus').textContent = '✅ All sensors normal';
      }
      
      document.getElementById('localIP').textContent = window.location.host;
      document.getElementById('wifiStatus').textContent = 'Connected';
    }
    
    async function refreshNow() {
      log('Manual refresh requested');
      await fetchData();
    }
    
    async function toggleBuzzer(state) {
      try {
        await fetch(`/cmd?buzzer=${state}`);
        log(`Buzzer turned ${state ? 'ON 🔊' : 'OFF 🔇'}`);
      } catch(e) {
        log('Buzzer command failed');
      }
    }
    
    async function testAlert() {
      log('Testing alert mode...');
      await fetch('/cmd?test=1');
    }
    
    async function fetchData() {
      try {
        const res = await fetch('/data');
        const data = await res.json();
        updateDisplay(data);
      } catch(e) {
        document.getElementById('wifiStatus').textContent = 'Disconnected';
        log('Connection lost');
      }
    }
    
    // Auto-refresh every 2 seconds
    setInterval(fetchData, 2000);
    fetchData(); // Initial load
  </script>
</body>
</html>
  )=====";
  
  server.send(200, "text/html", html);
}

void handleDataJSON() {
  String json = "{";
  json += "\"mq135_ppm\":";
  json += String(mq135_ppm, 1);
  json += ",\"mq7_raw\":";
  json += String(mq7_raw);
  json += ",\"sound_db\":";
  json += String(sound_db);
  json += ",\"alertActive\":";
  json += (alertActive ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleCmd() {
  if (server.hasArg("buzzer")) {
    bool state = server.arg("buzzer") == "1";
    digitalWrite(BUZZER_PIN, state);
    Serial.println("Web→Buzzer: " + String(state ? "ON" : "OFF"));
    server.send(200, "text/plain", "Buzzer " + String(state ? "ON" : "OFF"));
  } else if (server.hasArg("test")) {
    alertActive = true;
    server.send(200, "text/plain", "Test alert triggered");
  } else {
    server.send(400, "text/plain", "?buzzer=0/1 or ?test=1");
  }
}
