
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

static BLEUUID serviceUUID("dc3fd440-1308-4edd-9053-f97ea77683ab");
static BLEUUID charUUID("abd34827-ae5f-4ff6-b11c-9e4413e2b1ec");
static BLEUUID charUUID2("ee3a4157-0a3d-4f48-8466-2a3974b937b9");
String My_BLE_Address = "cc:50:e3:b6:97:a2";

#define STAGE_0 35
#define STAGE_1 32
#define STAGE_2 28
#define STAGE_3 20

#define FROSTBITE_30_MINUTES -25
#define FROSTBITE_10_MINUTES -35
#define FROSTBITE_5_MINUTES -45

bool deviceConnected = false;
int connection_missed_counter = 0;

//String server_address "cc:50:e3:b6:97:a2";

int wind_speed[15];
int wind_speed_index = 0;


static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;

BLEScan* pBLEScan; //Name the scanning device as pBLEScan
BLEScanResults foundDevices;

static BLEAddress *Server_BLE_Address;
String Scaned_BLE_Address;

bool connectToserver (BLEAddress pAddress){
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    // Connect to the BLE Server.
    pClient->connect(pAddress);
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService != nullptr)
    {
      Serial.println(" - Found our service");
      return true;
    }
    else
    return false;

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic != nullptr)
      Serial.println(" - Found our characteristic");

      return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks 
{
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.printf("Scan Result: %s \n", advertisedDevice.toString().c_str());
      Server_BLE_Address = new BLEAddress(advertisedDevice.getAddress());
      
      Scaned_BLE_Address = Server_BLE_Address->toString().c_str();
      
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

  Serial.println("Configuring BLE");
  BLEDevice::init("ESP32 Client");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  foundDevices = pBLEScan->start(5, false);
  Serial.println("Configuration complete");

  if (Scaned_BLE_Address == My_BLE_Address && connected == false)
    {
      Serial.println("Found Device :-)... connecting to Server as client");
       if (connectToserver(*Server_BLE_Address))
      {
        connected = true;  
      }
      else
      {
      Serial.println("Connection failed");
      }
    }

  Serial.println("Populating wind speed lookup table");
  for (int i=0;i<15;i++){
    wind_speed[i] = i*5;
  }

  Serial.println("Complete");

  Serial.println("MCU clock frequency: ");
  Serial.println(ESP.getCpuFreqMHz());
  delay(5000);
  
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

  
  //Serial.println(Temp1);
  //Serial.println(Temp2);

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
  //Serial.println(wind_chill_factor);
 
  if (wind_chill_factor>FROSTBITE_30_MINUTES){
    frostbite_danger_level = 0;
  } else if (wind_chill_factor>FROSTBITE_10_MINUTES){
    frostbite_danger_level = 1;
  } else if (wind_chill_factor>FROSTBITE_5_MINUTES){
    frostbite_danger_level = 2;
  } else {
    frostbite_danger_level = 3;
  }

  if (connected) {

     if (frostbite_danger_level>0 or hypothermia_stage>0)
    {
      Serial.println("about to write value");
      String frostbiteString = String(frostbite_danger_level);
      pRemoteCharacteristic->writeValue(frostbiteString.c_str(), frostbiteString.length());
    }

  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
    connection_missed_counter+=1;
  }

  
}
