#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <DHT.h>
#include "wifi_setup.h"
#include "sensors.h"
#include "actuators.h"
#include "firebase.h"
#include "lcd.h"
#include "pomodoro.h"
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
#include "phone_dock.h"
#include "esp32-hal-log.h"

//  Timers 
static unsigned long tSense   = 0; 
static unsigned long tPush    = 0;   
static unsigned long tPull    = 0;  
static unsigned long tConfig  = 0;  
static unsigned long tLcd     = 0;   
static unsigned long lastTick = 0;  
// NTP
const char* ntpServer        = "pool.ntp.org";
const long  gmtOffset_sec    = 7 * 3600;   
const int   daylightOffset_sec = 0;

static void initNTP() {
    Serial.print("[NTP] Syncing...");
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
        Serial.println("\n[NTP] FAILED – light auto mode may not work");
    }
}


static bool  s_prevDoorOpen   = false;
static unsigned long tDoorAlert = 0;        
#define DOOR_ALERT_COOLDOWN_MS  60000UL     

//  LCD mode 
static void refreshLcd() {
    if (v_doorOpen) {
        lcdShowDoorAlert();
        return;
    }
    if (pomoState != POMO_IDLE) {
        int m, s;
        pomodoroGetTimeLeft(m, s);
        lcdShowPomodoro(m, s, pomodoroIsWork());   // UC2 + UC6
    } else {
        lcdShowSensors();                           // UC2
    }
}

//  setup 
void setup() {
    esp_log_level_set("*", ESP_LOG_ERROR);       
    esp_log_level_set("WebServer", ESP_LOG_NONE);
    esp_log_level_set("wifi", ESP_LOG_NONE);
    esp_log_level_set("httpd", ESP_LOG_NONE);
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== Smart Home ESP32 ===");
    
    //dht.begin(); 
    //setLogLevel(ARDUHAL_LOG_LEVEL_ERROR);
    actuatorsInit();    
    sensorsInit();     
    lcdInit();          // UC2: LCD 16×2
    phoneDockInit();    // UC9

    // WiFi
    if (!wifiConnect()) {
        Serial.println("[WIFI] FAILED – reboot in 5s");
        delay(5000);
        ESP.restart();
    }

    initNTP();

    // Device node = MAC address
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    DEVICE_NODE = "/devices/" + mac;
    Serial.printf("[DEV] Node: %s\n", DEVICE_NODE.c_str());

    // Firebase
    if (!firebaseAnonSignIn()) {
        Serial.println("[FB] signin FAIL – restart");
        delay(2000);
        ESP.restart();
    }

    firebasePullConfig();
    firebasePullActuators();

    Serial.println("[SYS] Ready!");

    // Main loop
    while (true) {
        unsigned long now = millis();
        Serial.println("temperature: " + String(v_temp));
        Serial.println("humidity: " + String(v_humi));
        // UC1 Read sensor
        if (now - tSense >= 2000) {
            tSense = now;
            readSensors();
        }

        // UC1 Push Firebase 
        if (now - tPush >= 5000) {
            tPush = now;
            firebasePatchAll();
        }

        // UC3/UC5/UC6: Pull actuator commands 3s
        if (now - tPull >= 3000) {
            tPull = now;
            firebasePullActuators();
        }

        // Config 15s
        if (now - tConfig >= 15000) {
            tConfig = now;
            firebasePullConfig();
        }

        // UC2: LCD refresh 1s
        if (now - tLcd >= 1000) {
                tLcd = now;

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("T:");
                lcd.print(v_temp);
                lcd.print("C");

                lcd.setCursor(0, 1);
                lcd.print("H:");
                lcd.print(v_humi);
                lcd.print("%");
            }
        

        if (now - lastTick >= 1000) {
            lastTick = now;
            unsigned long nowSec = now / 1000;

            // UC3/UC4/UC5
            actuatorsStateMachineUpdate(nowSec);

            // UC6: Pomodoro
            pomodoroUpdate();

            // UC9: Dock 
            phoneDockUpdate();

            // UC7 + UC8: 
            if (v_doorOpen && !s_prevDoorOpen) {
                Serial.println("[UC7] Door OPENED!");
                if (now - tDoorAlert > DOOR_ALERT_COOLDOWN_MS) {
                    tDoorAlert = now;
                    firebaseSendDoorAlert();
                }
            } else if (!v_doorOpen && s_prevDoorOpen) {
                Serial.println("[UC7] Door CLOSED");
            }
            s_prevDoorOpen = v_doorOpen;
        }
    }
}

void loop() {
}
