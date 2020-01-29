
/* Scan for I2C address?

*/

#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <math.h>
#include <xtensa/hal.h>

#define DS_Address_1 0x48
#define DS_Address_2 0x49

#define HYPOTHERMIA_LED 17
#define FROSTBITE_LED 16

#define SERVICE_UUID            "dc3fd440-1308-4edd-9053-f97ea77683ab"
#define CHARACTERISTIC_UUID_1   "abd34827-ae5f-4ff6-b11c-9e4413e2b1ec"
#define CHARACTERISTIC_UUID_2   "ee3a4157-0a3d-4f48-8466-2a3974b937b9" 

#define STAGE_0 35
#define STAGE_1 32
#define STAGE_2 28
#define STAGE_3 20

#define FROSTBITE_30_MINUTES -25
#define FROSTBITE_10_MINUTES -35
#define FROSTBITE_5_MINUTES -45

bool deviceConnected = false;
int connection_missed_counter = 0;

int wind_speed[15];
int wind_speed_index = 0;



BLECharacteristic hypothermiaCharacteristic(
  CHARACTERISTIC_UUID_1, 
  BLECharacteristic::PROPERTY_READ | 
  BLECharacteristic::PROPERTY_NOTIFY
);

BLECharacteristic frostbiteCharacteristic(
  CHARACTERISTIC_UUID_2, 
  BLECharacteristic::PROPERTY_READ | 
  BLECharacteristic::PROPERTY_NOTIFY
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
  byte configuration;
  byte error;
  Serial.begin(9600);
  Serial.println(" ");
  delay(100);

  Wire.begin(21,22);
  Wire.setClock(100000);
  delay(1000);

  Serial.println("Configuring Temperature Sensor #1");
  Wire.beginTransmission(DS_Address_1);
  Wire.write(0xAC);
  Wire.write(0x03);
  error = Wire.endTransmission();
  delay(20);  
  if(error==0){
    Serial.println("Configuration complete");
  }
  else{
    Serial.println("Error: ");
    Serial.print(error);
  }

  Serial.println("Configuring Temperature Sensor #2");
  Wire.beginTransmission(DS_Address_2);
  Wire.write(0xAC);
  Wire.write(0x03);
  error = Wire.endTransmission();
  delay(20);  
  if(error==0){
    Serial.println("Configuration complete");
  }
  else{
    Serial.println("Error: ");
    Serial.print(error);
  }

  delay(100);


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

  Serial.println("Populating wind speed lookup table");
  for (int i=0;i<15;i++){
    wind_speed[i] = i*5;
  }

  Serial.println("Complete");

  Serial.println("MCU clock frequency: ");
  Serial.println(ESP.getCpuFreqMHz());
  delay(5000);

  pinMode(HYPOTHERMIA_LED, OUTPUT);
  pinMode(FROSTBITE_LED, OUTPUT);
}
 
void loop() {
  //digitalWrite(LED, HIGH);

  
  byte Temp1 = 0;
  byte Temp15 = 0;
  byte Temp2 = 0;
  byte Temp25 = 0;
  Wire.beginTransmission(DS_Address_1);
  Wire.write(0xEE);
  Wire.endTransmission();

  Wire.beginTransmission(DS_Address_2);
  Wire.write(0xEE);
  Wire.endTransmission();

  
  delay(800);
  
  Wire.beginTransmission(DS_Address_1);
  Wire.write(0xAA);
  Wire.endTransmission();

  Wire.beginTransmission(DS_Address_2);
  Wire.write(0xAA);
  Wire.endTransmission();

  Wire.requestFrom(DS_Address_1,2);
  Temp1 = Wire.read();
  Temp15 = Wire.read();

  Wire.requestFrom(DS_Address_2,2);
  Temp2 = Wire.read();
  Temp25 = Wire.read();

  
  Serial.println(Temp1);
  Serial.println(Temp2);

  int hypothermia_stage = 0;
  int frostbite_danger_level = 0;
  
  if (Temp1>STAGE_0)
  {
    hypothermia_stage = 0;
  } else if (Temp1>STAGE_1){
    hypothermia_stage = 1;
  } else if (Temp1>STAGE_2){
    hypothermia_stage = 2;
  } else if (Temp1>STAGE_3){
    hypothermia_stage = 3;
  } else {
    hypothermia_stage = 4;
  }

  int V = wind_speed[wind_speed_index];
  wind_speed_index+=1;
  if (wind_speed_index==15)
  {
    wind_speed_index = 0;
  }
  float wind_chill_factor = 13.12 + 0.6215*Temp2 - 11.37*pow(V, 0.16) +0.3965*Temp2*pow(V, 0.16);
  Serial.println(wind_chill_factor);
 
  if (wind_chill_factor>FROSTBITE_30_MINUTES){
    frostbite_danger_level = 0;
  } else if (wind_chill_factor>FROSTBITE_10_MINUTES){
    frostbite_danger_level = 1;
  } else if (wind_chill_factor>FROSTBITE_5_MINUTES){
    frostbite_danger_level = 2;
  } else {
    frostbite_danger_level = 3;
  }

  unsigned time1 = xthal_get_ccount();

  if (frostbite_danger_level>0 or hypothermia_stage>0)
  {
    if(deviceConnected){
      Serial.println("Send BLE data");
      hypothermiaCharacteristic.setValue(hypothermia_stage);
      hypothermiaCharacteristic.notify();
      frostbiteCharacteristic.setValue(frostbite_danger_level);
      frostbiteCharacteristic.notify();
      connection_missed_counter = 0;
      digitalWrite(FROSTBITE_LED, LOW);
      digitalWrite(HYPOTHERMIA_LED, LOW);
    } else {
      connection_missed_counter+=1;
      if (frostbite_danger_level>0)
      {
        digitalWrite(FROSTBITE_LED, HIGH);
      }
      if (hypothermia_stage>0)
      {
        digitalWrite(HYPOTHERMIA_LED, HIGH);
      }
    }
  }
  else{
    digitalWrite(FROSTBITE_LED, LOW);
    digitalWrite(HYPOTHERMIA_LED, LOW);
  }
  unsigned time2 = xthal_get_ccount();
  unsigned BLE_TIME = time2-time1;
  Serial.println(BLE_TIME);
  
}
