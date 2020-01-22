#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID            "dc3fd440-1308-4edd-9053-f97ea77683ab"
#define CHARACTERISTIC_UUID_1   "abd34827-ae5f-4ff6-b11c-9e4413e2b1ec"
#define CHARACTERISTIC_UUID_2   "ee3a4157-0a3d-4f48-8466-2a3974b937b9" 


bool deviceConnected = false;
int connection_missed_counter = 0;


BLECharacteristic hypothermiaCharacteristic(
  CHARACTERISTIC_UUID_1, 
  BLECharacteristic::PROPERTY_WRITE
);

BLECharacteristic frostbiteCharacteristic(
  CHARACTERISTIC_UUID_2, 
  BLECharacteristic::PROPERTY_WRITE
);


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  
  Serial.begin(9600);

  Serial.println("Configuring BLE");
  BLEDevice::init("ESP32");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pService->addCharacteristic(&hypothermiaCharacteristic);
  pService->addCharacteristic(&frostbiteCharacteristic);
  
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.println("Configuration complete");

}

void loop() {
  if(deviceConnected)
  {
    Serial.println(hypothermiaCharacteristic.getValue().c_str());
    Serial.println(frostbiteCharacteristic.getValue().c_str());
    Serial.println("------------------");
  }

  delay(500);

}
