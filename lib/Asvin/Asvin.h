/**
 * asvin.h
 * @author Rohit Bohara
 *
 * Copyright (c) 2019 asvin.io. All rights reserved.
 */
#ifndef ASVIN_H_
#define ASVIN_H_

#include <Arduino.h> 
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "HTTPUpdate.h"
#include <WiFiClientSecure.h>
#include <WiFiClient.h>

#ifndef DEBUG_ASVIN_UPDATE
#define DEBUG_ASVIN_UPDATE
 //#define DEBUG_ASVIN_UPDATE(...) Serial.printf( __VA_ARGS__ )
#endif

#ifndef DEBUG_ASVIN_UPDATE
#define DEBUG_ASVIN_UPDATE(...)
#endif

class Asvin
{
public:
  Asvin(void);
  ~Asvin(void);
  String registerDevice(const String mac, String currentFwVersion, String token, int& httpCode);
  String checkRollout(const String mac, const String currentFwVersion, String token, int& httpCode);
  String authLogin(String customer_key, String device_signature, long unsigned int timestamp, int& httpCode);
  String getBlockchainCID(const String firmwareID, String token, int& httpCode);
  String checkRolloutSuccess(const String mac, const String currentFwVersion, String token, const String rollout_id, int& httpCode);
  t_httpUpdate_return downloadFirmware(String token, const String cid);


private:

  const String registerURL = "https://app.vc.asvin.io/api/device/register";
  const String checkRolloutURL = "https://app.vc.asvin.io/api/device/next/rollout";
  const String checkRolloutSuccessURL = "https://app.vc.asvin.io/api/device/success/rollout";
  const String authserverLoginURL = "https://app.auth.asvin.io/auth/login";
  const String bcGetFirmwareURL = "https://app.besu.asvin.io/firmware/get";
  const String ipfsDownloadURL = "https://app.ipfs.asvin.io/firmware/download";


};
#endif
