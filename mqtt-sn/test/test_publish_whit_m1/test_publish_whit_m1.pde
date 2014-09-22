#include <WaspXBee802.h>
#include <WaspMqttSN.h>

const char *gw_mac = "0013A2004061D041";
int topicId = 1;
int qos = -1;

void setup() {
  mqttSN.set_gw_mac(gw_mac);
  mqttSN.obtain_sensorMac();
  
  USB.ON();
  USB.printf("-------------------------------\nTest of MQTT-SN:publish with QoS = -1\n-------------------------------\n");
  
  xbee802.ON();
}

void loop() {
  int ret = -1, resPub;
  
  //PUBLISH TO A TOPIC
  resPub = mqttSN.publish(topicId, MQTTSN_TOPIC_TYPE_PREDEFINED, "Hello World from Waspmote, QoS-1", qos, 0, 0);
  if(resPub == 1)
    USB.println("---- publish send");
    
  //WAIT FOR 5 SECONDS
  delay(5000);
    
  if (resPub)
    ret = 1;
   
  if(ret == 1)
    USB.println("All Test Passed!!");
  else
    USB.println("Test Failed!!");
    
  USB.println("restarting...");  
  USB.println("_____________________________\n");
  
  Utils.setLED(LED0, LED_ON);
  Utils.setLED(LED1, LED_ON);
  delay(500);
  Utils.setLED(LED0, LED_OFF);
  Utils.setLED(LED1, LED_OFF);
  
  delay(5000);
}
