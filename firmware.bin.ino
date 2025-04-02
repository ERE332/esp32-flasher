#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

Preferences preferences;
AsyncWebServer server(80);

// Default AP credentials
const char* ap_ssid = "CyberSec_AP";
const char* ap_password = "StrongP@ss123!";

// HTML, CSS a JavaScript
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>CyberSec Toolkit</title>
    <style>
        body { font-family: Arial; background: #1a1a1a; color: #00ff00; }
        .container { max-width: 800px; margin: 0 auto; padding: 20px; }
        .menu { background: #0d0d0d; padding: 15px; border-radius: 5px; }
        button { background: #006600; color: white; border: none; padding: 10px; margin: 5px; cursor: pointer; }
        button:hover { background: #009900; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Cyber Security Toolkit</h1>
        <div class="menu">
            <button onclick="showSection('wifi')">WiFi Config</button>
            <button onclick="showSection('portscan')">Port Scanner</button>
            <button onclick="showSection('passcheck')">Password Check</button>
        </div>
        
        <div id="wifi" class="section">
            <h3>WiFi Configuration</h3>
            <input type="text" id="ssid" placeholder="SSID">
            <input type="password" id="password" placeholder="Password">
            <button onclick="saveWiFi()">Save</button>
        </div>

        <div id="portscan" class="section" style="display:none;">
            <h3>Port Scanner</h3>
            <input type="text" id="targetIP" placeholder="Target IP">
            <button onclick="startScan()">Scan Ports</button>
            <div id="scanResults"></div>
        </div>

        <div id="passcheck" class="section" style="display:none;">
            <h3>Password Strength Check</h3>
            <input type="password" id="passwordInput">
            <button onclick="checkPassword()">Check</button>
            <div id="passwordResult"></div>
        </div>
    </div>

    <script>
        function showSection(sectionId) {
            document.querySelectorAll('.section').forEach(div => {
                div.style.display = 'none';
            });
            document.getElementById(sectionId).style.display = 'block';
        }

        async function saveWiFi() {
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            await fetch('/savewifi?ssid='+encodeURIComponent(ssid)+'&password='+encodeURIComponent(password));
            alert('Credentials Saved! Rebooting...');
        }

        async function startScan() {
            const ip = document.getElementById('targetIP').value;
            const response = await fetch('/portscan?ip='+ip);
            const results = await response.text();
            document.getElementById('scanResults').innerHTML = results;
        }

        function checkPassword() {
            const pass = document.getElementById('passwordInput').value;
            let strength = 0;
            
            if (pass.length >= 8) strength++;
            if (pass.match(/[A-Z]/)) strength++;
            if (pass.match(/[0-9]/)) strength++;
            if (pass.match(/[^A-Za-z0-9]/)) strength++;
            
            const result = document.getElementById('passwordResult');
            result.innerHTML = `Password Strength: ${strength}/4`;
        }
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    preferences.begin("wifi-creds", false);
    
    // Pokus o připojení k uložené WiFi
    String saved_ssid = preferences.getString("ssid", "");
    String saved_pass = preferences.getString("password", "");
    
    if(saved_ssid != "") {
        WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());
        delay(5000);
        
        if(WiFi.status() != WL_CONNECTED) {
            startAP();
        }
    } else {
        startAP();
    }

    // Web server routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    server.on("/savewifi", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("ssid") && request->hasParam("password")) {
            preferences.putString("ssid", request->getParam("ssid")->value());
            preferences.putString("password", request->getParam("password")->value());
            request->send(200, "text/plain", "OK");
            delay(2000);
            ESP.restart();
        }
    });

    server.on("/portscan", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("ip")) {
            String ip = request->getParam("ip")->value();
            String result = "Scan results for " + ip + ":\n";
            
            // Jednoduchý port scanner (běžné porty)
            int ports[] = {21, 22, 23, 80, 443, 3389};
            for(int port : ports) {
                WiFiClient client;
                if(client.connect(ip.c_str(), port)) {
                    result += "Port " + String(port) + ": OPEN\n";
                    client.stop();
                } else {
                    result += "Port " + String(port) + ": CLOSED\n";
                }
                delay(100);
            }
            request->send(200, "text/plain", result);
        }
    });

    server.begin();
}

void startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
}

void loop() {}