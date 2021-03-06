#include <WaspXBee802.h>
#include <WaspMqttSN.h>

const char *gw_mac = "0013A2004061D041";

void setup() {
  mqttSN.set_debug_on();
  mqttSN.set_gw_mac(gw_mac);
  mqttSN.obtain_sensorMac();
  
  USB.ON();
  USB.printf("-------------------------------\nTest of MQTT-SN:subscribe()\n-------------------------------\n");
  
  xbee802.ON();
}

void loop() {
  int ret = -1, resCon, resCack, resPingreq, resPingresp, resDis;
  
  //CONNECT TO MOSQUITTO
  resCon = mqttSN.connect();
  if (resCon == 1)
    USB.println("---- connect send");
    
  resCack = mqttSN.recv_connack();
  if (resCack == 1)
    USB.println("---- connack received");
    
  //WAIT FOR 5 SECONS
  delay(5000);
  
  resPingreq = mqttSN.pingreq();
  if (resPingreq == 1)
    USB.println("---- ping send");
    
  resPingresp = mqttSN.recv_ping();
  if (resPingresp == 1)
    USB.println("---- ping received");
    
  //WAIT FOR 5 SECONDS
  delay(5000);

  resDis = mqttSN.disconnect();
  if (resDis == 1)
    USB.println("---- disconnect send");
  
  if (resCon && resCack && resPingreq && resPingresp && resDis)
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
