#ifndef __SERVER_MQTTSN__
#define __SERVER_MQTTSN__

#include <stdint.h>
#include "mqtt.h"
#include "utils.h"

#define MAX_TOPICS 10;

enum mqttsn_types {
  MQTTSN_TYPE_CONNECT = (0x04),
  MQTTSN_TYPE_CONNACK = (0x05),
  MQTTSN_TYPE_REGISTER = (0x2E),
  MQTTSN_TYPE_REGACK = (0x0B),
  MQTTSN_TYPE_PUBLISH = (0x0C),
  MQTTSN_TYPE_PUBACK = (0x0D),
  MQTTSN_TYPE_PUBCOMP = (0x0E),
  MQTTSN_TYPE_PUBREC = (0x0F),
  MQTTSN_TYPE_PUBREL = (0x10),
  MQTTSN_TYPE_SUBSCRIBE = (0x12),
  MQTTSN_TYPE_SUBACK = (0x13),
  MQTTSN_TYPE_UNSUBSCRIBE = (0x14),
  MQTTSN_TYPE_UNSUBACK = (0x15),
  MQTTSN_TYPE_PINGREQ = (0x16),
  MQTTSN_TYPE_PINGRESP = (0x17),
  MQTTSN_TYPE_DISCONNECT = (0x18)
};

enum mqttsn_flags {
  MQTTSN_FLAG_DUP = (0x80),
  MQTTSN_FLAG_QoS0 = (0x00),
  MQTTSN_FLAG_QoS1 = (0x20),
  MQTTSN_FLAG_QoS2 = (0x40),
  MQTTSN_FLAG_QoSm1 = (0x60),
  MQTTSN_FLAG_RETAIN = (0x10),
  MQTTSN_FLAG_WILL = (0x08),
  MQTTSN_FLAG_CLEAN_SESSION = (0x04)
};

enum mqttsn_type_topics {
  MQTTSN_TOPIC_TYPE_NORMAL = (0x00),
  MQTTSN_TOPIC_TYPE_PREDEFINED = (0x01),
  MQTTSN_TOPIC_TYPE_SHORT = (0x02)
};

enum mqttsm_return_codes {
  ACCEPTED = (0x00),
  REJECTED_CONGESTION = (0x01),
  REJECTED_INVALID_TOPIC_ID = (0x02),
  REJECTED_NOT_SUPPORTED = (0x03)
};

enum mqttsn_ack_sizes {
  CONNACK_SIZE = 3,
  SUBACK_SIZE = 8,
  UNSUBACK_SIZE = 4,
  PUBACK_SIZE = 7,
  PUBREC_SIZE = 4,
  PUBCOMP_SIZE = 4,
  REGACK_SIZE = 7,
  PING_SIZE = 2
};

extern uint16_t msgid;
extern uint16_t topic_id_count;

int mqttsn2mqtt(char *mqttsn, broker_handle_t *broker, int msgtype, char *mac);
void mqtt2mqttsn(const char *mac);
void register_handler(char *mqttsn, char *mac);
uint8_t store_topicId(char *topicName, char *mac);
void send_regack(uint16_t topic_id, uint8_t msg_idMSB, uint8_t msg_idLSB, uint8_t return_code, const char *mac);
char* getTopicName_byID(char *mac, uint16_t topicID);
char* process_topicIdType(char *mqttsn, char *mac, uint8_t msgtype);
void initTopics_byDefault(char *mac);

#endif
