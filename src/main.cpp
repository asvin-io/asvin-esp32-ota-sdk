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

// NTP variables
const char* ntpServer = "europe.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;

String key = "3";
String firmware_version = "1.0.0";


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

void loop(){
  /*
    -Connect to WiFi
      -Login To Auth Server
        -Register Device 
          -CheckRollout
            -Get CID from BlockChain server
              -Download Firmware from IPFS server and update the ESP
                -Check if Rollout was sucessful 
    
  */
  
  if (WiFi.status() == WL_CONNECTED){ //Check WiFi connection status
      Serial.println("Wifi Connected !");
      delay(500);
      Serial.println("Getting Unix timestamp from NTP");
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //init and get the time
      struct tm timeinfo;
      while(!getLocalTime(&timeinfo)) {
      Serial.print("..\n");
      delay(500);
      }
      time_t now;
      time(&now);
      unsigned long timestr = now;
      Serial.println(timestr);

      //HMAC mbedtls
      char payload[10+strlen(device_key.c_str())] = "";
      byte hmacResult[32];
      ltoa(timestr, payload, 10);
      strcat(payload, device_key.c_str());    
      mbedtls_md_context_t ctx;
      mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    
      mbedtls_md_init(&ctx);
      mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
      mbedtls_md_hmac_starts(&ctx, (const unsigned char *) customer_key.c_str(), strlen(customer_key.c_str()));
      mbedtls_md_hmac_update(&ctx, (const unsigned char *) payload, strlen(payload));
      mbedtls_md_hmac_finish(&ctx, hmacResult);
      mbedtls_md_free(&ctx);
          
      String device_signature = "";
      for(int i= 0; i< sizeof(hmacResult); i++){
          char str[3];
          sprintf(str, "%02x", (int)hmacResult[i]);
          //Serial.print(str);
          device_signature += str;
      }
      Serial.print("Hash: ");
      Serial.println(device_signature);
  
      delay(2000);
      HTTPClient http;
      String mac = WiFi.macAddress();
      Asvin asvin;
      int httpCode;

      // ......Get OAuth Token.................. 
      Serial.println("Get OAuth Token ");
      
      String response = asvin.authLogin(device_key, device_signature, timestr, httpCode);
      Serial.print("response: "); Serial.println(response);
      Serial.println("Parsing Auth Code ");
      DynamicJsonDocument doc(1000);
      DeserializationError error = deserializeJson(doc, response);
      if(!doc["token"])
      {
        Serial.println("Did not receive the token!!");
        return;
      }
      String authToken = doc["token"];
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }

      Serial.println(authToken);

      // Register Device 

      String result = asvin.RegisterDevice(mac, firmware_version, authToken, httpCode);
      char buff[result.length() + 1];
      result.toCharArray(buff, result.length() + 1);
      //Serial.print("Buffer --> ");
      Serial.println(buff);
      Serial.println(httpCode);
      
      if (httpCode == 200){
        Serial.println("Device Registered: OK ");

        //  -CheckRollout
        String resultCheckout = asvin.CheckRollout(mac, firmware_version, authToken, httpCode);
        Serial.println("checkRollout");
        char buff[resultCheckout.length() + 1];
        resultCheckout.toCharArray(buff, resultCheckout.length() + 1);
        Serial.println(buff);
        
        if (httpCode == 200){
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
          // -Get CID from BlockChain server
          String CidResponse = asvin.GetBlockchainCID(firmwareID, authToken, httpCode);
          char cidbuff[CidResponse.length() + 1];
          CidResponse.toCharArray(cidbuff, CidResponse.length() + 1);
          Serial.println(cidbuff);

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
            // -Download Firmware from IPFS server
            t_httpUpdate_return ret = asvin.DownloadFirmware(authToken, cid);
            switch (ret)
            {
            case HTTP_UPDATE_FAILED:
              Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
              break;
            case HTTP_UPDATE_NO_UPDATES:
              Serial.println("HTTP_UPDATE_NO_UPDATES");
              break;
            case HTTP_UPDATE_OK:
              Serial.println("HTTP_UPDATE_OK");
              break;
            }
            if (httpCode == 200){
              Serial.println("Firmware updated");
            }
            // if (httpCode == 200)
            // {
            //   // check if rollout successfull 
            //   Serial.println("http 200");
            //   String resultCheckout = asvin.CheckRolloutSuccess(mac, firmware_version, authToken, rolloutID, httpCode);
            //   char buff[resultCheckout.length() + 1];
            //   resultCheckout.toCharArray(buff, resultCheckout.length() + 1);
            //   Serial.println(buff);
            // }
          }

          // -Check if Rollout was sucessful 
      }
    }
  }
}
