/**
 * asvin.cpp
 * @author Rohit Bohara
 *
 * Copyright (c) 2019 asvin.io. All rights reserved.
 */
#include "Asvin.h"
#include <Arduino.h>



Asvin::Asvin(void) {

}
Asvin::~Asvin(void) {

}


String Asvin::authLogin(String device_key, String device_signature, long unsigned int timestamp, int& httpCode) {
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  HTTPClient http;
  http.begin(*client, authserverLoginURL);
  http.addHeader(F("Content-Type"), "application/json");
  DynamicJsonDocument doc(500);
  doc["device_key"] = device_key;
  doc["device_signature"] = device_signature;
  doc["timestamp"] = timestamp;
  String payload;
  serializeJson(doc, payload);
  char buff[payload.length() + 1];
  payload.toCharArray(buff, payload.length() + 1);
  DEBUG_ASVIN_UPDATE("[asvin auth server] Auth Server Login : %s\n", buff);
  httpCode = http.POST(payload);   //Send the request
  delay(1000);
  String res = http.getString();  //Get the response payload
  delay(1000);
  http.end();  //Close connection to asvin server
  return res;
}


String Asvin::registerDevice(const String mac, String currentFwVersion, String token, int& httpCode) {
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  HTTPClient http;
  http.begin(*client, registerURL);
  http.addHeader(F("Content-Type"), "application/json");
  http.addHeader(F("x-access-token"), token);
  DynamicJsonDocument doc(500);
  doc["mac"] = mac;
  doc["firmware_version"] = currentFwVersion;
  String payload;
  serializeJson(doc, payload);
  char buff[payload.length() + 1];
  payload.toCharArray(buff, payload.length() + 1);
  DEBUG_ASVIN_UPDATE("[asvin Version Controller] Register Device  : %s\n", buff);
  httpCode = http.POST(payload);   //Send the request
  delay(1000);
  String res = http.getString();  //Get the response payload
  delay(1000);
  http.end();  //Close connection to asvin server
  return res;
}


String Asvin::checkRollout(const String mac, const String currentFwVersion, String token, int& httpCode) {
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  HTTPClient http;
  http.begin(*client, checkRolloutURL);
  http.addHeader(F("Content-Type"), "application/json");
  http.addHeader(F("x-access-token"), token);
  DynamicJsonDocument doc(500);
  doc["mac"] = mac;
  doc["firmware_version"] = currentFwVersion;
  String payload;
  serializeJson(doc, payload);
  char buff[payload.length() + 1];
  payload.toCharArray(buff, payload.length() + 1);
  DEBUG_ASVIN_UPDATE("[asvinUpdate] Next RollOut ---> : %s\n", buff);
  httpCode = http.POST(payload); //Send the request
  delay(1000);
  String res = http.getString();  //Get the response payload
  delay(1000);
  http.end();  //Close connection to asvin server
  return res;
}




String Asvin::getBlockchainCID(const String firmwareID, String token, int& httpCode) {
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  HTTPClient http;
  http.begin(*client, bcGetFirmwareURL);
  http.addHeader(F("Content-Type"), "application/json");
  http.addHeader(F("x-access-token"), token);
  DynamicJsonDocument doc(256);
  doc["id"] = firmwareID;
  String payload;
  serializeJson(doc, payload);
  char buff[payload.length() + 1];
  payload.toCharArray(buff, payload.length() + 1);
  DEBUG_ASVIN_UPDATE("[asvinUpdate] Blockchain Login : %s\n", buff);
  httpCode = http.POST(payload);   //Send the request
  delay(1000);
  String res = http.getString();  //Get the response payload
  delay(1000);
  http.end();  //Close connection to asvin server
  return res;
}


String Asvin::checkRolloutSuccess(const String mac, const String currentFwVersion, String token, const String rollout_id, int& httpCode) {
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  HTTPClient http;
  http.begin(*client, checkRolloutSuccessURL);
  http.addHeader(F("Content-Type"), "application/json");
  http.addHeader(F("x-access-token"), token);
  DynamicJsonDocument doc(256);
  doc["mac"] = mac;
  doc["firmware_version"] = currentFwVersion;
  String payload;
  serializeJson(doc, payload);
  char buff[payload.length() + 1];
  payload.toCharArray(buff, payload.length() + 1);
  DEBUG_ASVIN_UPDATE("[asvinUpdate] Check Rollout Success  : %s\n", buff);
  httpCode = http.POST(payload);   //Send the request
  delay(1000);
  String res = http.getString();  //Get the response payload
  delay(1000);
  http.end();  //Close connection to asvin server
  return res;
}



t_httpUpdate_return Asvin::downloadFirmware(String token, const String cid) {
  std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure);
  StaticJsonDocument<80> doc;
  doc["cid"] = cid;
  String payload;
  const String currentVersion = "1.0.0";
  serializeJson(doc, payload);
  char buff[payload.length() + 1];
  payload.toCharArray(buff, payload.length() + 1);
  DEBUG_ASVIN_UPDATE("[asvinUpdate] Download firmware HTTP payload : %s\n", buff);
  t_httpUpdate_return res = httpUpdate.update(*client, ipfsDownloadURL, payload, token, currentVersion);
  return res;
}


