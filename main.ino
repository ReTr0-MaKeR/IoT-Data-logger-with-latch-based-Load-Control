/*
Author: Md. Tanvir Hassan
Department of EEE
Daffodil International University
Date of development: 10th May 2024
*/

#include <Arduino.h>    //Library for Arduino 
#include <WiFi.h>       // Library for Wifi uses 
#include "time.h"        // Library for using time function
#include <ESP_Google_Sheet_Client.h>  //Libray for Google sheet

#include <GS_SDHelper.h>  // Library for google sheet data handling

#define WIFI_SSID "Test-Project"
#define WIFI_PASSWORD "Test-Project"

#define PROJECT_ID "Your_Project_ID" // Taken from personal google script account
#define CLIENT_EMAIL "Your_Client_Email" // Same as before 
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nYOUR_PRIVATE_KEY\n-----END PRIVATE KEY-----\n";//same as before 
const char spreadsheetId[] = "10tDBylsn3qXCu7t9EUrwYP8iNWKHPQHslTZERy1MYyw"; //taken from google sheet !must be public 

unsigned long lastTime = 0;
unsigned long timerDelay = 30000; // equivalen to 30 second ~used to delay the data sending process

void tokenStatusCallback(TokenInfo info);

const char* ntpServer = "pool.ntp.org";// taken from google script 

unsigned long epochTime;

String getTime()  // this whole function was used for fetching the realtime date and time .
{
    time_t now;// for fetching the  real time 
    struct tm timeinfo;
    time(&now);
    gmtime_r(&now, &timeinfo);
    timeinfo.tm_hour += 6; 
    if (timeinfo.tm_hour >= 24) {
        timeinfo.tm_hour -= 24;
        timeinfo.tm_mday++; 
    }
    char formattedTime[50];
    strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", &timeinfo);// to get this following format 5/13/2024 18:58:00
    return String(formattedTime);
}

#define gasPin 35 //  for the gas sensor, used D0 pin not the A0 pin
#define lightPin 34 // for the light sesnor 
#define button1Pin 25// used to control the light or led
#define button2Pin 26// same as before 
#define led1Pin 32  // used for led 
#define led2Pin 33

bool led1State = false;  // forcefully turing off both leds
bool led2State = false;  // forcefully turing off both leds

bool button1State = false;  // forcefully turing off both buttons or switches 
bool button2State = false;   // forcefully turing off both buttons or switches 

void setup()
{
    Serial.begin(115200); // used to initate the serial monitor for debuging purpose 
    Serial.println();

    pinMode(gasPin,INPUT);  // used to declare the pins , if they are input or output type 
    pinMode(lightPin,INPUT);
    pinMode(button1Pin, INPUT_PULLUP); // we used pull up to constantly propting the processor that button is at HIGH or True state
    pinMode(button2Pin, INPUT_PULLUP); // we used pull up to constantly propting the processor that button is at HIGH or True state
    pinMode(led1Pin, OUTPUT);
    pinMode(led2Pin, OUTPUT);

    configTime(0, 0, "time.google.com"); // just ensuring that time is correct with bd time 
    const char *tz = "BDT-6";
    setenv("TZ", tz, 1);
    tzset();

    GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION); // prompting the google sheet that we will use it from now on 

    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // used for connecting to the wifi network 

    Serial.print("Connecting to Wi-Fi"); 
    //********************************************************************************************************************
    while (WiFi.status() != WL_CONNECTED) {  //  used for debugging , to know if wifi is connected or not 
      Serial.print(".");
      delay(1000); //resting for 1 second 
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP()); // for debugging , to know the ip adress 
    Serial.println();
 //************************************************************************************************************************
    GSheet.setTokenCallback(tokenStatusCallback);
    GSheet.setPrerefreshSeconds(10 * 60);
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);// google will verify the user from those credentials 
}

void loop(){
    bool ready = GSheet.ready();// ask google sheet it it is ready or not to take the data 

    if (ready && millis() - lastTime > timerDelay)// if it is ready and 30 second delay is over the following work will be done 
    {
    //***********************************************************************************************
        lastTime = millis();// keep track of the time 
        
        FirebaseJson response;   // prompting the google server 
        String currentTime = getTime(); getting the time 

        
        int gasValue = digitalRead(gasPin); // reading the gas sensor 
        String gasStatus;
        if (gasValue == 1) {
            gasStatus = "Fresh";
        } else {
            gasStatus = "Harmful";
        }
        
        int lightValue = digitalRead(lightPin);// reading light sensor 
        String lightStatus;
        if (lightValue == 0) {
            lightStatus = "light";
        } else {
            lightStatus = "dark";
        }
        String led1Status = led1State ? "On" : "Off";// monitoring the led status if they are one 
        String led2Status = led2State ? "On" : "Off";// monitoring the led status if they are one
        //*************************************************by this time we got all the 4 data we need to send *************************************
        FirebaseJson valueRange;
        
        valueRange.add("majorDimension", "COLUMNS"); // setting up the coloumns 
        valueRange.set("values/[0]/[0]", currentTime);// sending time and date to the 1st coloumn 
        valueRange.set("values/[1]/[0]", gasStatus);  // sending gas data to 2nd coloumn 
        valueRange.set("values/[2]/[0]", lightStatus);
        valueRange.set("values/[3]/[0]", led1Status);
        valueRange.set("values/[4]/[0]", led2Status);
        //--------------------------------------------------------------------------------------------- for debugging*--------------- 
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
        /--------------------------------------------------------------------------------------------------------------------------------
    }
//***************************************************************************************************************
    if (digitalRead(button1Pin) == LOW && !button1State) // creating software latch and controlling led 
    {
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
