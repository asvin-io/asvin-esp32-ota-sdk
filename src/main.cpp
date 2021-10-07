/*
 * ESP32 OTA
 *
 * The sketch was developed for a prototype consist of ESP32 board.
 * The board connects to asvin platform and perform OTA firmware updates.
 *
 * Written by Rahul Karade, Apache-2.0 License
 */

#include <Arduino.h>
#include <Stream.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "Asvin.h"
#include <time.h> //ESP32 NTP
#include <mbedtls/md.h> //ESP32 crypto
#include <WiFi.h>
#include <credentials.h>
#include "WiFiManager.h"

 //#define DEBUG_MY_UPDATE

#ifndef DEBUG_MY_UPDATE
#define DEBUG_MY_UPDATE(...) Serial.printf( __VA_ARGS__ )
#endif

 // NTP variables
const char* ntpServer = "europe.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;

String key = "3";
String firmware_version = "1.0.0";

// Get credentials from asvin platform
String customer_key = CUSTOMER_KEY;
String device_key = DEVICE_KEY;

bool device_registered = false;

void setup()
{
  Serial.begin(115200); //Serial connection
  delay(500);

  WiFiManager wifiManager;
  //Uncomment below code to reset onboard wifi credentials
  //wifiManager.resetSettings();
  wifiManager.autoConnect("AutoConnectAP");
  /*
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to the WiFi(%s)\n", ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
   delay(2000);
   Serial.println("..");
  }
  */
  Serial.println("Connected to the WiFi network");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //init and get the time

}

void loop() {
  /*
    -Connect to WiFi
      -Login To Auth Server
        -Register Device
          -CheckRollout
            -Get CID from BlockChain server
              -Download Firmware from IPFS server and update the ESP
                -Check if Rollout was sucessful

  */

  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    //Serial.println("Wifi Connected !");
    delay(500);
    //Serial.println("Getting Unix timestamp from NTP");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //init and get the time
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo)) {
      Serial.print("..\n");
      delay(500);
    }
    time_t now;
    time(&now);
    unsigned long timestr = now;
    //Serial.println(timestr);

    //HMAC mbedtls
    char payload[10 + strlen(device_key.c_str())] = "";
    byte hmacResult[32];
    ltoa(timestr, payload, 10);
    strcat(payload, device_key.c_str());
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char*)customer_key.c_str(), strlen(customer_key.c_str()));
    mbedtls_md_hmac_update(&ctx, (const unsigned char*)payload, strlen(payload));
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);

    String device_signature = "";
    for (int i = 0; i < sizeof(hmacResult); i++) {
      char str[3];
      sprintf(str, "%02x", (int)hmacResult[i]);
      //Serial.print(str);
      device_signature += str;
    }
    //Serial.print("Hash: ");
    //Serial.println(device_signature);

    delay(2000);
    HTTPClient http;
    String mac = WiFi.macAddress();
    Asvin asvin;
    int httpCode;

    // ......Get OAuth Token.................. 
    Serial.println("Get OAuth Token ");

    String response = asvin.authLogin(device_key, device_signature, timestr, httpCode);
    //Serial.print("response: ");
    //Serial.println("Parsing Auth Code ");
    DynamicJsonDocument doc(1000);
    DeserializationError error = deserializeJson(doc, response);
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    if (!doc["token"])
    {
      Serial.println("Did not receive the token!!");
      return;
    }
    String authToken = doc["token"];
    //Serial.println(authToken);

    // Register Device 
    if (device_registered) {
      httpCode = 200;
    }
    else {
      Serial.println("--Register Device");
      String result = asvin.registerDevice(mac, firmware_version, authToken, httpCode);
      char buff[result.length() + 1];
      result.toCharArray(buff, result.length() + 1);
      //Serial.print("Buffer --> ");
      //Serial.println(buff);
      //Serial.println(httpCode);
    }
    if (httpCode == 200) {
      Serial.println("Device Registered: OK ");

      //  CheckRollout
      Serial.println("--Check Next Rollout");
      String resultCheckout = asvin.checkRollout(mac, firmware_version, authToken, httpCode);
      char buff[resultCheckout.length() + 1];
      resultCheckout.toCharArray(buff, resultCheckout.length() + 1);
      //Serial.println(buff);

      if (httpCode == 200) {
        Serial.println("Next Rollout: OK");
        DynamicJsonDocument doc(1000);
        DeserializationError error = deserializeJson(doc, resultCheckout);

        if (error)
        {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }
        String firmwareID = doc["firmware_id"];
        String rolloutID = doc["rollout_id"];
        if (rolloutID == "null") {
          Serial.println("No Rollout available");
          Serial.println("---------------------");
          delay(1000 * 3);
          return;
        }

        // Get CID from BlockChain server
        Serial.println("--Get Firmware Info from Blockchain");
        String CidResponse = asvin.getBlockchainCID(firmwareID, authToken, httpCode);
        char cidbuff[CidResponse.length() + 1];
        CidResponse.toCharArray(cidbuff, CidResponse.length() + 1);
        //Serial.println(cidbuff);

        if (httpCode == 200) {
          Serial.println("CID Resonse : OK");
          DynamicJsonDocument doc(500);
          DeserializationError error = deserializeJson(doc, CidResponse);
          if (error)
          {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return;
          }
          String cid = doc["cid"];
          if (cid.length() == 0) {
            Serial.println("No CID available");
            return;
          }

          // Download Firmware from IPFS server
          Serial.println("--Download Firmware from IPFS and Install");
          t_httpUpdate_return ret = asvin.downloadFirmware(authToken, cid);
          switch (ret) {
          case HTTP_UPDATE_FAILED:
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            break;
          case HTTP_UPDATE_NO_UPDATES:
            Serial.println("HTTP_UPDATE_NO_UPDATES");
            break;
          case HTTP_UPDATE_OK:
            Serial.println("HTTP_UPDATE_OK");
            // check if rollout successfull 
            //Serial.println("--Update Rollout");
            String resultCheckout = asvin.checkRolloutSuccess(mac, firmware_version, authToken, rolloutID, httpCode);
            char buff[resultCheckout.length() + 1];
            resultCheckout.toCharArray(buff, resultCheckout.length() + 1);
            //Serial.println(buff);
            if (httpCode == 200) {
              //Serial.println("Update Rollout : OK");
              Serial.println("--Restart Device and Apply Update : OK");
              ESP.restart();
            }
            else {
              Serial.println("Rollout Update Error!!");
            }
            break;
          }
        }
        else {
          DEBUG_MY_UPDATE("IPFS Error!!\n");
        }
      }
      else {
        DEBUG_MY_UPDATE("Next Rollout Error!!\n");
      }
    }
    else {
      DEBUG_MY_UPDATE("Device Registration Error!!\n");
    }
  }
  else {
    DEBUG_MY_UPDATE("Problem with WiFi!!");
  }
}
