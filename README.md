# IoT-Project

Wearable application for assessing risk of frostbite/hypothermia in low-temperature environments. 
One temperature sensor close to the wearer's skin detects core temperature to monitor for hypothermia, which begins to set in 
below 35 degrees Celsius. A second temperature sensor outside the clothes measures the environmental temperature. 
Combined with measured wind speed, we calculate wind chill and therefore the risk for frostbite in exposed body parts. 
If either of the measurements fall below a threshold, an alert is forwarded to the user's mobile phone. 
If no Bluetooth connection is found, there should be an alternative way to alert the user, e.g. by using a blinking LED.

MCU is an ESP32 D0WD Q6
