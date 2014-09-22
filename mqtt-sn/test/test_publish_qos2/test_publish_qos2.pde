#include <WaspXBee802.h>
#include <WaspMqttSN.h>

const char *gw_mac = "0013A2004061D041";
uint8_t qos = 2;
uint8_t dup = 0;
uint8_t retain = 0;

void setup() {
  mqttSN.set_gw_mac(gw_mac);
  mqttSN.obtain_sensorMac();
  
  USB.ON();
  USB.printf("-------------------------------\nTest of MQTT-SN:publish with QoS 2()\n-------------------------------\n");
  
  xbee802.ON();
}

void loop() {
  int ret = -1, resCon, resCack, resPub, resPrel, resPrec, resPcomp;
  
  //CONNECT TO MOSQUITTO
  resCon = mqttSN.connect();
  if (resCon == 1)
    USB.println("---- connect send");
    
  resCack = mqttSN.recv_connack();
  if (resCack == 1)
    USB.println("---- connack received");
    
  //WAIT FOR 5 SECONS
  delay(5000);
  
  //PUBLISH TO A TOPIC
  resPub = mqttSN.publish(0x01, MQTTSN_TOPIC_TYPE_PREDEFINED, "Hello World from Waspmote", qos, dup, retain);
  if(resPub == 1)
    USB.println("---- publish send");
  
  if(qos == 2) {
    resPrec = mqttSN.recv_pubrec();
    if(resPrec == 1)
      USB.println("---- puback received");
    resPrel = mqttSN.pubrel();
    if(resPrel == 1)
      USB.println("---- pubrel send");
    resPcomp = mqttSN.recv_pubcomp();
    if(resPcomp == 1)
      USB.println("---- pubcomp received");
  }
  
  //WAIT FOR 5 SECONDS
  delay(5000);
    
  if (resCon && resCack && resPub && resPrel && resPrec && resPcomp)
    ret = 1;
   
  if(ret == 1)
    USB.println("All Test Passed!!");
  else
    USB.println("Test Fail!!");
    
  USB.println("restarting...");  
  USB.println("_____________________________\n");
  
  Utils.setLED(LED0, LED_ON);
  Utils.setLED(LED1, LED_ON);
  delay(500);
  Utils.setLED(LED0, LED_OFF);
  Utils.setLED(LED1, LED_OFF);
  
  delay(5000);
}
