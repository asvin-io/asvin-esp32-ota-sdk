# asvin-esp32-ota-sdk
ESP32 SDK to integrate asvin libraries for over the air update through asvin platform

## Tutorial of ESP32 OTA using asvin platform

### Setup:
1. Rename credentials_copy.h to credentials.h
2. Update `customer_key`, `device_key` 
3. Then build the application and flash it to ESP32 
4. The application uses WiFiManager to connect to the WiFi. Please follow the instructions given in `https://github.com/tzapu/WiFiManager` to connect to WiFi

### API Flow
1. Connect to WiFi
2. Login To Auth Server
3. Register Device 
4. CheckRollout
5. Get CID from BlockChain server
6. Download Firmware from IPFS server and update the ESP
7. Check if Rollout was sucessful
