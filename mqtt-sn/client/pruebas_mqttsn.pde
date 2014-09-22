#include <WaspXBee802.h>
//#include <minunit.h>
#include <WaspMqttSN.h>

const char *gw_mac = "0013A2004061D041";
//int tests_run = 0;
 
void setup() {
  mqttSN.set_debug_on();
  mqttSN.set_gw_mac(gw_mac);
  mqttSN.obtain_sensorMac();
  
  USB.ON();
  USB.println("example of connect");
  USB.OFF();
}

void loop() {
  /*char *result = all_tests();
  
  if (result != 0) {
    USB.printf("%s\n", result);
  }
  else {
    USB.printf("ALL TESTS PASSED\n");
  }
  printf("Tests run: %d\n", tests_run);
  */
  
  /*
  mqttSN.connect();
  delay(2000);
  */
  USB.ON();
  int res = mqttSN.recv_connack();
  if(res == 1) {
    USB.printf("## %d data received\n", res);
  } else {
    USB.printf("## data not received\n");
  }
  
  Utils.setLED(LED0, LED_ON);
  Utils.setLED(LED1, LED_ON);
  delay(1000);
  Utils.setLED(LED0, LED_OFF);
  Utils.setLED(LED1, LED_OFF);
  
  delay(10000);
}
/*
static char* test_connect() {
  int res = mqttSN.connect();
  mu_assert("error, connect not send", res == -1);
  return 0;
}

static char* all_tests() {
  mu_run_test(test_connect);
  return 0;
}
*/
