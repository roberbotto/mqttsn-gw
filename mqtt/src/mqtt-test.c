#include "CUnit/Basic.h"
#include "CUnit/CUnit.h"
#include "CUnit/Automated.h"
#include "mqtt.h"

broker_handle_t *broker;

/*SUITE*/
int init_suite(void)
{
  if(((broker = (broker_handle_t*)malloc(sizeof(broker_handle_t))) != NULL) &&
    (mqtt_init(broker, "localhost", "1883")) >= 0)
    {
      return 0;
    }
  else {
    return -1;
  }
}

int clean_suite(void)
{
  close(broker->sockfd);
  free(broker);
  broker=NULL;
  return 0;
}

/* Clean-session conexion Test */
void testCONNECT_clean(void)
{
  mqtt_set_client_id(broker, "mqtt_clean");
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

/* Unclean-session conexion Test */
void testCONNECT_unClean(void)
{
  mqtt_init(broker, "localhost", "1883");
  mqtt_set_client_id(broker, "mqtt_unclean");
  broker->clean_session = 0;
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

/* Username and password conexion Test */
void testCONNECT_userpass(void)
{
  mqtt_init(broker, "localhost", "1883");
  mqtt_set_client_id(broker, "mqtt_userpass");
  mqtt_set_userpass(broker, "botto", "botto");
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

void testSUBSCRIBE_UNSUBSCRIBE_topicQoS0(void)
{
  mqtt_init(broker, "localhost", "1883");
  mqtt_set_client_id(broker, "mqtt_unsubscribe");
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_subscribe(broker, "a/c", 0) );
  CU_ASSERT_TRUE( mqtt_recv_suback(broker) );
  CU_ASSERT_TRUE( mqtt_send_unsubscribe(broker, "a/c") );
  CU_ASSERT_TRUE( mqtt_recv_unsuback(broker) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

void testSUBSCRIBE_UNSUBSCRIBE_topicQoS1(void)
{
  mqtt_init(broker, "localhost", "1883");
  mqtt_set_client_id(broker, "mqtt_un-subscribe");
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_subscribe(broker, "a/c", 1) );
  CU_ASSERT_TRUE( mqtt_recv_suback(broker) );
  CU_ASSERT_TRUE( mqtt_send_unsubscribe(broker, "a/c") );
  CU_ASSERT_TRUE( mqtt_recv_unsuback(broker) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

void testPUBLISH_QoS0(void)
{
  mqtt_init(broker, "localhost", "1883");
  mqtt_set_client_id(broker, "mqtt_publish0");
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_publish(broker, "a/c", "Hello World!", 0, 0, 0) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

void testPUBLISH_QoS1(void)
{
  mqtt_init(broker, "localhost", "1883");
  mqtt_set_client_id(broker, "mqtt_publish1");
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_publish(broker, "a/c", "Hello World!", 1, 0, 0) );
  CU_ASSERT_TRUE( mqtt_recv_puback(broker) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

void testPUBLISH_QoS2(void)
{
  mqtt_init(broker, "localhost", "1883");
  mqtt_set_client_id(broker, "mqtt_publish1");
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_publish(broker, "a/c", "Hello World!", 2, 0, 0) );
  CU_ASSERT_TRUE( mqtt_recv_pubrec(broker) );
  CU_ASSERT_TRUE( mqtt_send_pubrel(broker) );
  CU_ASSERT_TRUE( mqtt_recv_pubcomp(broker) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

void testPUBLISH_dup(void)
{
  mqtt_init(broker, "localhost", "1883");
  mqtt_set_client_id(broker, "mqtt_publish_dup");
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_publish(broker, "a/d", "Hello World!", 0, 1, 0) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

void testPUBLISH_retain(void)
{
  mqtt_init(broker, "localhost", "1883");
  mqtt_set_client_id(broker, "mqtt_publish_retain");
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_publish(broker, "a/d", "Hello World!", 0, 0, 1) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

void testPING(void)
{
  mqtt_init(broker, "localhost", "1883");
  mqtt_set_client_id(broker, "mqtt_ping");
  CU_ASSERT_TRUE( mqtt_send_connect(broker) );
  CU_ASSERT_TRUE( mqtt_recv_connack(broker) );
  CU_ASSERT_TRUE( mqtt_send_ping(broker) );
  CU_ASSERT_TRUE( mqtt_recv_ping(broker) );
  CU_ASSERT_TRUE( mqtt_send_disconnect(broker) );
}

/* The main() function for setting up and running the tests.
 * Returns a CUE_SUCCESS on successful running, another
 * CUnit error code on failure.
 */
int main()
{
  CU_pSuite pSuite = NULL;

  /* initialize the CUnit test registry */
  if (CU_initialize_registry() != CUE_SUCCESS)
    return CU_get_error();

  /* add the suites to the registry */
  pSuite = CU_add_suite("General Suite", init_suite, clean_suite);
  if (pSuite == NULL) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* add the tests to the suites */
  if((CU_add_test(pSuite, "connect & disconnect()_clean", testCONNECT_clean) == NULL) ||
     (CU_add_test(pSuite, "connect & disconnect()_unclean", testCONNECT_unClean) == NULL) ||
     (CU_add_test(pSuite, "connect & disconnect()_userpass", testCONNECT_userpass) == NULL) ||
     (CU_add_test(pSuite, "subscribe & unsubscribe()_topicQoS0", testSUBSCRIBE_UNSUBSCRIBE_topicQoS0) == NULL) ||
     (CU_add_test(pSuite, "subscribe & unsubscribe()_topicQoS1", testSUBSCRIBE_UNSUBSCRIBE_topicQoS1) == NULL) ||
     (CU_add_test(pSuite, "publish()_QoS0", testPUBLISH_QoS0) == NULL) ||
     (CU_add_test(pSuite, "publish()_QoS1", testPUBLISH_QoS1) == NULL) ||
     (CU_add_test(pSuite, "publish()_QoS2", testPUBLISH_QoS2) == NULL) ||
     (CU_add_test(pSuite, "publish()_dup", testPUBLISH_dup) == NULL) ||
     (CU_add_test(pSuite, "publish()_retain", testPUBLISH_retain) == NULL) ||
     (CU_add_test(pSuite, "ping()", testPING) == NULL))
    {
      CU_cleanup_registry();
      return CU_get_error();
    }

  /* Run all tests using the CUnit Basic interface */
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();

  /* Run all tests using the automated interface
     CU_set_output_filename("mqtt-test");
     CU_automated_run_tests();
     CU_list_tests_to_file();*/

  CU_cleanup_registry();
  return CU_get_error();
}
