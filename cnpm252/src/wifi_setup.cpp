#include "wifi_setup.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include "esp_wifi.h"

bool wifiConnect() {
    Serial.println("[WiFi] Starting...");

    WiFi.mode(WIFI_STA);
    WiFi.begin("XD", "00000000");
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    esp_wifi_set_ps(WIFI_PS_NONE);

    WiFiManager wm;
    wm.setConfigPortalTimeout(180);

    wm.setWebServerCallback([](){});   // no-op callback

    bool ok = wm.autoConnect("SMARTHOME-SETUP");
    if (!ok) {
        Serial.println("[WiFi] Connect FAIL");
        return false;
    }

    wm.stopWebPortal();

    Serial.printf("[WiFi] OK  IP: %s\n", WiFi.localIP().toString().c_str());

    if (MDNS.begin("smarthome")) {
        Serial.println("[mDNS] http://smarthome.local");
    }
    return true;
}