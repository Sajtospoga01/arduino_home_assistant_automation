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
  "my_light"        // MQTT Client name
);
const String PREFIX = "homeassistant";

// Home Assitant device creation for MQTT discovery
HAMqttDevice light("My Nice Lamp", HAMqttDevice::LIGHT);

// Pin assignation
const int relayPin = D3;

// Current status of the light
bool lightOn = false;
struct LightValues {
  bool lightOn;
  int brightness;
};

LightValues lightValues = { .lightOn = false, .brightness = 100 };

void setup() 
{
  pinMode(relayPin, OUTPUT);
  
  //start serial
  Serial.begin(115200);
  Serial.println(light.getConfigTopic());

  //Creating device details
  Dictionary &device = *(new Dictionary());
  device.insert("identifiers","dim-arduino"+WiFi.macAddress());
  device.insert("manufacturer","me");
  device.insert("model","dimming_light");
  device.insert("name","dim_light_arduino");
  device.insert("sw_version","1.0.0");
  String devString = device.json();
  Serial.println(devString);

  // Setup the parameters of the device for MQTT discovery in HA
  light.enableAttributesTopic();
  light.addConfigVar("bri_stat_t", "~/br/state");
  light.addConfigVar("bri_cmd_t", "~/br/cmd");
  light.addConfigVar("device",devString);
  
  //client http access for update
  client.enableHTTPWebUpdater();
  client.setMaxPacketSize(512);
  //debugging enabled
  client.enableDebuggingMessages();
}

void onConnectionEstablished()
{
  // Subscribe to the command topic ("cmd_t") of this device.
  client.subscribe(light.getCommandTopic(), [] (const String &payload)
  {
    // Turn the light on/off depending on the command received from Home Assitant
    if (payload.equals("ON"))
      lightValues.lightOn = true;
    else if (payload.equals("OFF"))
      lightValues.lightOn = false;

    // Confirm to Home Assitant that we received the command and updated the state.
    // HA will then update the state of the device in HA
    client.publish(light.getStateTopic(), payload);


  });
    // Brightness command received
  client.subscribe(light.getTopic() + "/br/cmd", [] (const String &payload)
  {
    
    lightValues.brightness = payload.toInt();
    client.publish(light.getTopic() + "/br/state", payload);
   
  });
  
  // Subscribe to Home Assitant connection status events.
  client.subscribe("homeassistant/status", [] (const String &payload)
  {
    // When the status of Home assistant get online, publish the device config
    if (payload.equals("online"))
    {
      client.publish(light.getConfigTopic(), light.getConfigPayload());
      
      // After sending the device config to HA, wait a little to allow HA to create the entity.

      client.executeDelayed(5*1000, []() {
        client.publish(light.getStateTopic(), lightValues.lightOn ? "ON" : "OFF");
        client.publish(light.getTopic() + "/br/state", String(lightValues.brightness));

        light
          .clearAttributes()
          .addAttribute("IP", WiFi.localIP().toString());
        client.publish(light.getAttributesTopic(), light.getAttributesPayload());
      });
    }
  });
}

void loop() 
{
  client.loop();

  if(lightValues.lightOn)
    analogWrite(relayPin, lightValues.brightness);
  else
    analogWrite(relayPin, 0);
}
