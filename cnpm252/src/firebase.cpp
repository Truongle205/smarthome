#include "firebase.h"
#include "sensors.h"
#include "actuators.h"
#include "phone_dock.h"
#include "pomodoro.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// Credentials 
const char* FIREBASE_API_KEY = "AIzaSyB4s0jvomb4RqeHi28EpDKws3V1fmto3Kg";
const char* FIREBASE_DB_URL  = "https://yolohome-7e643-default-rtdb.firebaseio.com";

String g_idToken;
String g_localId;
String DEVICE_NODE;

// Default configuration values (can be overridden from Firebase)
String lightOnTime        = "23:00";   // UC4: Light ON time (23:00)
String lightOffTime       = "06:00";   // UC4: Light OFF time (06:00)
float  fanTempThreshold   = 30.0f;     // UC5: Temperature threshold for fan AUTO mode
int    phoneAlertDelaySec = 30;        // UC9: Delay before phone alert (seconds)
int    phoneDockThreshold = 800;       // UC9: Threshold to detect phone on dock
int    pomoWorkSec        = 25 * 60;   // UC6: Pomodoro work duration
int    pomoBreakSec       = 5  * 60;   // UC6: Pomodoro break duration
String gmailWebhookUrl    = "";        // UC8: Gmail webhook URL (set via Firebase)

// HTTP helper functions
static bool httpPOST(const String& url, const String& json, String* resp = nullptr) {
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, url)) return false;
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(json);
    if (resp) *resp = http.getString();
    http.end();
    return (code >= 200 && code < 300);
}

static bool httpPATCH(const String& url, const String& json, String* resp = nullptr) {
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, url)) return false;
    http.addHeader("Content-Type", "application/json");
    int code = http.sendRequest("PATCH", (uint8_t*)json.c_str(), json.length());
    if (resp) *resp = http.getString();
    http.end();
    return (code >= 200 && code < 300);
}

static bool httpGET(const String& url, String* resp = nullptr) {
    WiFiClientSecure client; client.setInsecure();
    HTTPClient http;
    if (!http.begin(client, url)) return false;
    int code = http.GET();
    if (resp) *resp = http.getString();
    http.end();
    return (code >= 200 && code < 300);
}

// UC: Anonymous sign-in to Firebase 
bool firebaseAnonSignIn() {
    String url  = "https://identitytoolkit.googleapis.com/v1/accounts:signUp?key="
                  + String(FIREBASE_API_KEY);
    String body = "{\"returnSecureToken\":true}";
    String resp;

    if (!httpPOST(url, body, &resp)) {
        Serial.println("[FB] signUp HTTP FAIL");
        return false;
    }

    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, resp)) {
        Serial.println("[FB] signUp JSON FAIL");
        return false;
    }

    g_idToken = doc["idToken"].as<String>();
    g_localId = doc["localId"].as<String>();
    Serial.printf("[FB] Anon UID: %s\n", g_localId.c_str());
    return true;
}

// Push all device states (sensors + actuators + pomodoro) to Firebase
bool firebasePatchAll() {
    if (DEVICE_NODE.isEmpty() || g_idToken.length() < 10) return false;

    String url = String(FIREBASE_DB_URL) + DEVICE_NODE + ".json?auth=" + g_idToken;

    StaticJsonDocument<768> doc;
    doc["ts"] = (uint32_t)(millis() / 1000); // Timestamp (seconds since boot)

    // Sensors data
    JsonObject s = doc.createNestedObject("sensors");
    s["temp"] = isnan(v_temp) ? -1.0f : v_temp;
    s["humi"] = isnan(v_humi) ? -1.0f : v_humi;
    s["lux"]  = v_lux;
    s["ldrDock"] = v_ldrDock;
    s["doorOpen"] = v_doorOpen ? 1 : 0;
    s["phoneOnDock"] = phoneIsOnDock() ? 1 : 0;

    // Actuator states
    JsonObject a = doc.createNestedObject("actuators");
    a["light"]     = s_light ? 1 : 0;
    a["lightMode"] = (lightMode == LIGHT_MODE_AUTO) ? "auto" : "manual";
    a["buzzer"]    = s_buzz ? 1 : 0;

    // UC5: Fan state
    const char* fanStr = "off";
    switch (fanState) {
        case FAN_MANUAL: fanStr = "manual"; break;
        case FAN_AUTO:   fanStr = "auto";   break;
        default:         fanStr = "off";    break;
    }
    a["fan"] = fanStr;

    // UC6: Pomodoro state
    JsonObject p = doc.createNestedObject("pomodoro");
    const char* pomoStr = "idle";
    switch (pomoState) {
        case POMO_WORK:  pomoStr = "work";  break;
        case POMO_BREAK: pomoStr = "break"; break;
        default:         pomoStr = "idle";  break;
    }
    p["state"] = pomoStr;

    int pMin, pSec;
    pomodoroGetTimeLeft(pMin, pSec);
    p["minLeft"] = pMin;
    p["secLeft"] = pSec;

    String json;
    serializeJson(doc, json);

    if (!httpPATCH(url, json)) {
        Serial.println("[FB] PATCH FAIL – re-signing in");
        if (firebaseAnonSignIn()) return httpPATCH(url, json);
        return false;
    }
    return true;
}

// Pull actuator commands from Firebase
void firebasePullActuators() {
    if (DEVICE_NODE.isEmpty() || g_idToken.length() < 10) return;

    String url = String(FIREBASE_DB_URL) + DEVICE_NODE + "/actuators.json?auth=" + g_idToken;
    String resp;
    if (!httpGET(url, &resp)) return;

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, resp)) return;

    // UC3/UC4: Light mode (auto/manual)
    if (!doc["lightMode"].isNull()) {
        const char* m = doc["lightMode"];
        if      (!strcmp(m, "auto"))   lightMode = LIGHT_MODE_AUTO;
        else if (!strcmp(m, "manual")) lightMode = LIGHT_MODE_MANUAL;
    }

    // UC3: Light control (manual mode only)
    if (lightMode == LIGHT_MODE_MANUAL && !doc["light"].isNull()) {
        setLight(doc["light"].as<int>() != 0);
    }

    // Manual buzzer control
    if (!doc["buzzer"].isNull()) {
        setBuzzer(doc["buzzer"].as<int>() != 0);
    }

    // UC5: Fan mode
    if (!doc["fan"].isNull()) {
        String fm = doc["fan"].as<String>();
        if      (fm == "manual") fanSetMode(FAN_MANUAL);
        else if (fm == "auto")   fanSetMode(FAN_AUTO);
        else                     fanSetMode(FAN_OFF);
    }

    // UC6: Pomodoro command
    if (!doc["pomodoroCmd"].isNull()) {
        String cmd = doc["pomodoroCmd"].as<String>();
        if      (cmd == "start") pomodoroStart();
        else if (cmd == "stop")  pomodoroStop();
    }
}

// Pull configuration parameters from Firebase
void firebasePullConfig() {
    if (DEVICE_NODE.isEmpty() || g_idToken.length() < 10) return;

    String url = String(FIREBASE_DB_URL) + DEVICE_NODE + "/config.json?auth=" + g_idToken;
    String resp;
    if (!httpGET(url, &resp)) return;

    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, resp)) return;

    if (doc.containsKey("lightOnTime")) {
        lightOnTime = doc["lightOnTime"].as<String>();
        Serial.printf("[CFG] lightOnTime: %s\n", lightOnTime.c_str());
    }
    if (doc.containsKey("lightOffTime")) {
        lightOffTime = doc["lightOffTime"].as<String>();
        Serial.printf("[CFG] lightOffTime: %s\n", lightOffTime.c_str());
    }

    if (doc.containsKey("fanTempThreshold")) {
        fanTempThreshold = doc["fanTempThreshold"].as<float>();
        Serial.printf("[CFG] fanTempThreshold: %.1f\n", fanTempThreshold);
    }

    if (doc.containsKey("pomoWorkSec")) {
        pomoWorkSec = doc["pomoWorkSec"].as<int>();
        Serial.printf("[CFG] pomoWorkSec: %d\n", pomoWorkSec);
    }
    if (doc.containsKey("pomoBreakSec")) {
        pomoBreakSec = doc["pomoBreakSec"].as<int>();
        Serial.printf("[CFG] pomoBreakSec: %d\n", pomoBreakSec);
    }

    // UC8: Gmail webhook URL
    if (doc.containsKey("gmailWebhookUrl")) {
        gmailWebhookUrl = doc["gmailWebhookUrl"].as<String>();
        Serial.printf("[CFG] gmailWebhookUrl set (%d chars)\n", gmailWebhookUrl.length());
    }

    if (doc.containsKey("phoneAlertDelaySec")) {
        phoneAlertDelaySec = doc["phoneAlertDelaySec"].as<int>();
        Serial.printf("[CFG] phoneAlertDelaySec: %d\n", phoneAlertDelaySec);
    }
    if (doc.containsKey("phoneDockThreshold")) {
        phoneDockThreshold = doc["phoneDockThreshold"].as<int>();
        Serial.printf("[CFG] phoneDockThreshold: %d\n", phoneDockThreshold);
    }
}

// UC8: Send door-open alert via Gmail webhook
void firebaseSendDoorAlert() {
    if (gmailWebhookUrl.isEmpty()) {
        Serial.println("[UC8] gmailWebhookUrl not configured!");
        return;
    }

    // Get current time
    struct tm timeinfo;
    char timeStr[32] = "unknown";
    if (getLocalTime(&timeinfo)) {
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }

    // JSON payload sent to webhook 
    StaticJsonDocument<256> doc;
    doc["event"]  = "DOOR_OPENED";
    doc["device"] = DEVICE_NODE;
    doc["time"]   = timeStr;
    doc["temp"]   = isnan(v_temp) ? -1.0f : v_temp;
    doc["humi"]   = isnan(v_humi) ? -1.0f : v_humi;

    String json;
    serializeJson(doc, json);

    Serial.printf("[UC8] Sending door alert: %s\n", json.c_str());

    if (httpPOST(gmailWebhookUrl, json)) {
        Serial.println("[UC8] Door alert sent OK");
    } else {
        Serial.println("[UC8] Door alert FAILED");
    }
}