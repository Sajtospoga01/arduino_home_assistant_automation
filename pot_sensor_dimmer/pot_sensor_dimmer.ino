#include "EspMQTTClient.h"
#include "HAMqttDevice.h"
#include <Dictionary.h>

// MQTT client setup
EspMQTTClient client(
  "BT-G6C28W",         // Wifi ssid
  "vNK6Nph74e9xdu",     // Wifi password
  "192.168.1.197",  // MQTT broker ip
  "homeassistant",     // MQTT username
  "jai5nor1Oawoh3ojeiFee9eiK8bu7woo3dieneafaidoqui6aijo1Iech5equ2ae", // MQTT password
  "my_sensor"        // MQTT Client name
);
unsigned long lastSentAt = millis();
double lastValue = 0;
HAMqttDevice potSensor("My pot sensor", HAMqttDevice::SENSOR);

//const int potPin = A0;

int potPrevValue = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("setup begin...");

  Serial.println(potSensor.getConfigTopic());

  //Creating device details
  Dictionary &device = *(new Dictionary());
  device.insert("identifiers", "dimmer-arduino" + WiFi.macAddress());
  device.insert("manufacturer", "me");
  device.insert("model", "dimmer");
  device.insert("name", "light_dimmer_arduino");
  device.insert("sw_version", "1.0.0");
  String devString = device.json();



  // Setup the parameters of the device for MQTT discovery in HA
  potSensor.enableAttributesTopic();
  potSensor.addConfigVar("device", devString);

  Serial.println("topics enabled...");
  client.enableHTTPWebUpdater();
  client.setMaxPacketSize(512);
  Serial.println("webupdater enabled...");
  client.enableDebuggingMessages();

}
void onConnectionEstablished()
{
  Serial.println("Connection established...");
  // Subscribe to the command topic ("cmd_t") of this device.
  client.subscribe("homeassistant/status", [] (const String & payload)
  {
    // When the status of Home assistant get online, publish the device config
    if (payload.equals("online"))
    {
      client.publish(potSensor.getConfigTopic(), potSensor.getConfigPayload());

      // After sending the device config to HA, wait a little to allow HA to create the entity.
      client.executeDelayed(5 * 1000, []() {
        client.publish(potSensor.getStateTopic(), String(lastValue));

        // Send the IP address of the device to HA
        potSensor
        .clearAttributes()
        .addAttribute("IP", WiFi.localIP().toString());

        client.publish(potSensor.getAttributesTopic(), potSensor.getAttributesPayload());
      });

    }
  });

}

void loop() {
  client.loop();

  if ((millis() - lastSentAt) >= 50) {
    lastSentAt = millis();
    int curValue = map(analogRead(0), 0, 1024, 0, 255);
    if (lastValue != curValue) {
      lastValue = curValue;
      if (client.isConnected()) {
        client.publish(potSensor.getStateTopic(), String(lastValue));
      }
      //Serial.println(lastValue);
    }




    // Supported data types:
    // uint32_t (uint16_t, uint8_t)
    // int32_t (int16_t, int8_t)
    // double
    // float
    // const char*
  }

}
