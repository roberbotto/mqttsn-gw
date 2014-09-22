#ifndef MQTTSN_H
#define MQTTSN_H

/*includes*/
#include <inttypes.h>
#include <WaspXBee802.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

/*definitions & declarations*/
#define SUCCESS                      1
#define MAX_PACKET_LENGTH            66

#define MQTTSN_PROTOCOL_VERSION      (0x01)
#define MQTTSN_TYPE_CONNECT          (0x04)
#define MQTTSN_TYPE_CONNACK          (0x05)
#define MQTTSN_TYPE_REGISTER         (0x2E) //ñapa, en realidad es 0x0a, es un apaño
#define MQTTSN_TYPE_REGACK           (0x0B)
#define MQTTSN_TYPE_PUBLISH          (0x0C)
#define MQTTSN_TYPE_PUBACK           (0x0D)
#define MQTTSN_TYPE_PUBCOMP          (0x0E)
#define MQTTSN_TYPE_PUBREC           (0x0F)
#define MQTTSN_TYPE_PUBREL           (0x10)
#define MQTTSN_TYPE_SUBSCRIBE        (0x12)
#define MQTTSN_TYPE_SUBACK           (0x13)
#define MQTTSN_TYPE_UNSUBSCRIBE      (0x14)
#define MQTTSN_TYPE_UNSUBACK         (0x15)
#define MQTTSN_TYPE_PINGREQ          (0x16)
#define MQTTSN_TYPE_PINGRESP         (0x17)
#define MQTTSN_TYPE_DISCONNECT       (0x18)

#define MQTTSN_FLAG_DUP              (0x80)
#define MQTTSN_FLAG_QoS0             (0x00)
#define MQTTSN_FLAG_QoS1             (0x20)
#define MQTTSN_FLAG_QoS2             (0x40)
#define MQTTSN_FLAG_QoSm1	     (0x60)
#define MQTTSN_FLAG_RETAIN           (0x10)
#define MQTTSN_FLAG_WILL             (0x08)
#define MQTTSN_FLAG_CLEAN_SESSION    (0x04)

#define MQTTSN_TOPIC_TYPE_NORMAL     (0x00)
#define MQTTSN_TOPIC_TYPE_PREDEFINED (0x01)
#define MQTTSN_TOPIC_TYPE_SHORT      (0x02)

#define MAX_TOPICS                    10

/*Recommended values for timers and counters. All timers are in seconds.*/
#define T_ADV 960
#define N_ADV 3
#define T_SEARCH_GW 5
#define T_GW_INFO 5
#define T_WAIT 360
#define T_RETRY 15
#define N_RETRY 5

enum ack_sizes {
  CONNACK_SIZE = 3,
  SUBACK_SIZE = 8,
  UNSUBACK_SIZE = 4,
  PUBACK_SIZE = 7,
  PUBREC_SIZE = 4,
  PUBCOMP_SIZE = 4,
  REGACK_SIZE = 7,
  PING_SIZE = 2
};

enum return_codes {
  ACCEPTED = (0x00),
  REJECTED_CONGESTION = (0x01),
  REJECTED_INVALID_TOPIC_ID = (0x02),
  REJECTED_NOT_SUPPORTED = (0x03)
};

/*struct to store the relationship beetwen topic name and topic id*/
struct topic {
  const char *name;
  uint16_t id;
};

/*class*/
class WaspMqttSN
{
  uint16_t id, msg_id, keep_alive;
  const char *gw_mac;
  uint8_t debug, clean_session, topic_count;
  char macHigh[10], macLow[11], client_id[23];
  uint8_t *mqttsn_send_buffer;
  uint8_t *mqttsn_recv_buffer;
  long previous;

  topic topic_table[MAX_TOPICS];

 private:
  const char* get_msgtype(uint8_t type_id);
  int check_msgtype(uint8_t type_id);
  int send_packet(uint8_t mqttsn_packet_size, uint8_t type_id);
  int recv_packet(uint8_t mqttsn_packet_size, uint8_t type_id);

 public:
  WaspMqttSN();
  ~WaspMqttSN(){};

  void set_debug_on(void);
  void set_gw_mac(const char *mac);
  void set_client_id(char *my_client_id);
  void set_keep_alive(uint16_t duration);
  void obtain_sensorMac(void);
  uint16_t look_up_topicId(const char* topicName);

  int connect(void);
  int disconnect(void);
  int disconnect(uint16_t duration);
  int pingreq(void);
  int pingreq_with_clientID(void);
  int subscribe_topicId(uint16_t topicId, uint8_t qos, uint8_t topicType);
  int subscribe_topicName(const char *topicName, uint8_t qos);
  int unsubscribe_topicId(uint16_t topicId, uint8_t topicType);
  int unsubscribe_topicName(const char *topicName);
  int publish(uint16_t topicId, uint8_t topicType, const char *msg, uint8_t qos, uint8_t dup, uint8_t retain);
  int pubrel(void);
  int register_topicName(const char *topicName);

  int recv_connack(void);
  int recv_suback(void);
  int recv_unsuback(void);
  int recv_puback(void);
  int recv_pubrec(void);
  int recv_pubcomp(void);
  int recv_ping(void);
  int recv_regack(void);

};

extern WaspMqttSN mqttSN;

#endif
