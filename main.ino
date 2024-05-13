/*
Author: Md. Tanvir Hassan
Department of EEE
Daffodil International University
Date of development: 10th May 2024
*/

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include <ESP_Google_Sheet_Client.h>

#include <GS_SDHelper.h>

#define WIFI_SSID "Your_WiFi_SSID"
#define WIFI_PASSWORD "Your_WiFi_Password"

#define PROJECT_ID "Your_Project_ID"
#define CLIENT_EMAIL "Your_Client_Email"

const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nYOUR_PRIVATE_KEY\n-----END PRIVATE KEY-----\n";

const char spreadsheetId[] = "Your_Spreadsheet_ID";

unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

void tokenStatusCallback(TokenInfo info);

const char* ntpServer = "pool.ntp.org";

unsigned long epochTime;

String getTime() {
    time_t now;
    struct tm timeinfo;
    time(&now);
    gmtime_r(&now, &timeinfo);
    timeinfo.tm_hour += 6; 
    if (timeinfo.tm_hour >= 24) {
        timeinfo.tm_hour -= 24;
        timeinfo.tm_mday++; 
    }
    char formattedTime[50];
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(formattedTime);
}

#define gasPin 35
#define lightPin 34
#define button1Pin 25
#define button2Pin 26
#define led1Pin 32
#define led2Pin 33

bool led1State = false;
bool led2State = false;

bool button1State = false;
bool button2State = false;

void setup(){
    Serial.begin(115200);
    Serial.println();

    pinMode(gasPin,INPUT);
    pinMode(lightPin,INPUT);
    pinMode(button1Pin, INPUT_PULLUP);
    pinMode(button2Pin, INPUT_PULLUP);
    pinMode(led1Pin, OUTPUT);
    pinMode(led2Pin, OUTPUT);

    configTime(0, 0, "time.google.com");
    const char *tz = "BDT-6";
    setenv("TZ", tz, 1);
    tzset();

    GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);

    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    GSheet.setTokenCallback(tokenStatusCallback);
    GSheet.setPrerefreshSeconds(10 * 60);
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}

void loop(){
    bool ready = GSheet.ready();

    if (ready && millis() - lastTime > timerDelay){
        lastTime = millis();
        FirebaseJson response;
        String currentTime = getTime();
        int gasValue = digitalRead(gasPin);
        String gasStatus;
        if (gasValue == 1) {
            gasStatus = "Fresh";
        } else {
            gasStatus = "Harmful";
        }
        int lightValue = digitalRead(lightPin);
        String lightStatus;
        if (lightValue == 0) {
            lightStatus = "light";
        } else {
            lightStatus = "dark";
        }
        String led1Status = led1State ? "On" : "Off";
        String led2Status = led2State ? "On" : "Off";
        FirebaseJson valueRange;
        valueRange.add("majorDimension", "COLUMNS");
        valueRange.set("values/[0]/[0]", currentTime);
        valueRange.set("values/[1]/[0]", gasStatus);
        valueRange.set("values/[2]/[0]", lightStatus);
        valueRange.set("values/[3]/[0]", led1Status);
        valueRange.set("values/[4]/[0]", led2Status);
        bool success = GSheet.values.append(&response, spreadsheetId, "Sheet1!A1", &valueRange);
        if (success){
            response.toString(Serial, true);
            valueRange.clear();
        }
        else{
            Serial.println(GSheet.errorReason());
        }
        Serial.println();
        Serial.println(ESP.getFreeHeap());
    }

    if (digitalRead(button1Pin) == LOW && !button1State) {
        led1State = !led1State;
        digitalWrite(led1Pin, led1State ? HIGH : LOW);
        button1State = true;
    } else if (digitalRead(button1Pin) == HIGH && button1State) {
        button1State = false;
    }

    if (digitalRead(button2Pin) == LOW && !button2State) {
        led2State = !led2State;
        digitalWrite(led2Pin, led2State ? HIGH : LOW);
        button2State = true;
    } else if (digitalRead(button2Pin) == HIGH && button2State) {
        button2State = false;
    }
}

void tokenStatusCallback(TokenInfo info){
    if (info.status == token_status_error){
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else{
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}
