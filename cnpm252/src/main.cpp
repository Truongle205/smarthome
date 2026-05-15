#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "wifi_setup.h"
#include "sensors.h"
#include "actuators.h"
#include "firebase.h"
#include "lcd.h"
#include "pomodoro.h"
#include "phone_dock.h"
#include "esp32-hal-log.h"

// Timers
static unsigned long tSense       = 0;
static unsigned long tPush        = 0;
static unsigned long tPull        = 0;
static unsigned long tConfig      = 0;
static unsigned long tLcd         = 0;
static unsigned long tOneSecond   = 0;
static unsigned long tDoorAlert   = 0;
static unsigned long tSerialLog   = 0;
static bool s_prevDoorOpen        = false;

static const unsigned long SENSE_INTERVAL_MS      = 2000UL;
static const unsigned long FIREBASE_PUSH_MS       = 5000UL;
static const unsigned long FIREBASE_PULL_MS       = 3000UL;
static const unsigned long CONFIG_PULL_MS         = 15000UL;
static const unsigned long LCD_REFRESH_MS         = 1000UL;
static const unsigned long DOOR_ALERT_COOLDOWN_MS = 60000UL;
static const unsigned long SERIAL_LOG_MS          = 5000UL;

// NTP: Vietnam UTC+7
static const char* ntpServer = "pool.ntp.org";
static const long gmtOffset_sec = 7 * 3600;
static const int daylightOffset_sec = 0;

static void initNTP() {
    Serial.print("[NTP] Syncing");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    time_t now = time(nullptr);
    int attempt = 0;
    while (now < 10000 && attempt < 20) {
        delay(500);
        now = time(nullptr);
        attempt++;
        Serial.print(".");
    }

    struct tm ti;
    if (now > 10000) {
        localtime_r(&now, &ti);
        Serial.printf("\n[NTP] OK: %02d:%02d:%02d\n", ti.tm_hour, ti.tm_min, ti.tm_sec);
    } else {
        Serial.println("\n[NTP] FAILED - light AUTO schedule may not work");
    }
}

static void refreshLcd() {
    if (v_doorOpen) {
        lcdShowDoorAlert();
        return;
    }

    if (pomoState != POMO_IDLE) {
        int m = 0, s = 0;
        pomodoroGetTimeLeft(m, s);
        lcdShowPomodoro(m, s, pomodoroIsWork());
        return;
    }

    lcdShowSensors();
}

static void handleDoorAlert(unsigned long now) {
    if (v_doorOpen && !s_prevDoorOpen) {
        Serial.println("[UC7] Door OPENED");
        if (now - tDoorAlert > DOOR_ALERT_COOLDOWN_MS || tDoorAlert == 0) {
            tDoorAlert = now;
            firebaseSendDoorAlert();
        }
    } else if (!v_doorOpen && s_prevDoorOpen) {
        Serial.println("[UC7] Door CLOSED");
    }
    s_prevDoorOpen = v_doorOpen;
}

static void serialLog(unsigned long now) {
    if (now - tSerialLog < SERIAL_LOG_MS) return;
    tSerialLog = now;

    Serial.printf("[SENSOR] temp=%.1f humi=%.1f lux=%d dock=%d door=%s\n",
                  v_temp, v_humi, v_lux, v_ldrDock, v_doorOpen ? "OPEN" : "CLOSED");
}

void setup() {
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set("WebServer", ESP_LOG_NONE);
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("httpd", ESP_LOG_NONE);

    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== Smart Farm - Yolo UNO ESP32-S3 ===");

    actuatorsInit();
    sensorsInit();
    lcdInit();
    phoneDockInit();

    if (!wifiConnect()) {
        Serial.println("[WiFi] FAILED - reboot in 5s");
        delay(5000);
        ESP.restart();
    }

    initNTP();

    String mac = WiFi.macAddress();
    mac.replace(":", "");
    DEVICE_NODE = "/devices/" + mac;
    Serial.printf("[DEV] Node: %s\n", DEVICE_NODE.c_str());

    if (!firebaseAnonSignIn()) {
        Serial.println("[FB] sign-in FAIL - reboot in 2s");
        delay(2000);
        ESP.restart();
    }

    readSensors();
    firebasePullConfig();
    firebasePullActuators();
    firebasePatchAll();
    refreshLcd();

    Serial.println("[SYS] Ready");
}

void loop() {
    const unsigned long now = millis();

    if (now - tSense >= SENSE_INTERVAL_MS) {
        tSense = now;
        readSensors();
    }

    if (now - tPull >= FIREBASE_PULL_MS) {
        tPull = now;
        firebasePullActuators();
    }

    if (now - tConfig >= CONFIG_PULL_MS) {
        tConfig = now;
        firebasePullConfig();
    }

    if (now - tPush >= FIREBASE_PUSH_MS) {
        tPush = now;
        firebasePatchAll();
    }

    if (now - tLcd >= LCD_REFRESH_MS) {
        tLcd = now;
        refreshLcd();
    }

    if (now - tOneSecond >= 1000UL) {
        tOneSecond = now;
        actuatorsStateMachineUpdate(now / 1000UL);
        pomodoroUpdate();
        phoneDockUpdate();
        handleDoorAlert(now);
    }

    serialLog(now);
    delay(5);
}
