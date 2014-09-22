#ifndef MQTT_H
#define MQTT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

#ifndef SUCCESS
#define SUCCESS                       1
#endif

#ifndef MAX_PACKET_LENGTH
#define MAX_PACKET_LENGTH             (255)
#endif

#define MQTT_PROTOCOL_VERSION         (0x03)

/*sizes*/
#define MQTT_SIZE_CONNACK             (4)
#define MQTT_SIZE_PUBACK              (4)
#define MQTT_SIZE_SUBACK              (5)
#define MQTT_SIZE_UNSUBACK            (4)
#define MQTT_SIZE_PINGRESP            (2)

/*messages types*/
#define MQTT_TYPE_CONNECT             (0x10)
#define MQTT_TYPE_CONNACK             (0x20)
#define MQTT_TYPE_PUBLISH             (0x30)
#define MQTT_TYPE_PUBACK              (0x40)
#define MQTT_TYPE_PUBREC              (0x50)
#define MQTT_TYPE_PUBREL              (0x60)
#define MQTT_TYPE_PUBCOMP             (0x70)
#define MQTT_TYPE_SUBSCRIBE           (0x80)
#define MQTT_TYPE_SUBACK              (0x90)
#define MQTT_TYPE_UNSUBSCRIBE         (0xA0)
#define MQTT_TYPE_UNSUBACK            (0xB0)
#define MQTT_TYPE_PINGREQ             (0xC0)
#define MQTT_TYPE_PINGRESP            (0xD0)
#define MQTT_TYPE_DISCONNECT          (0xE0)

/*connect flags*/
#define MQTT_CONNECT_CLEAN_SESSION    (0x02)
#define MQTT_CONNECT_WILL             (0x04)
#define MQTT_CONNECT_WILL_QoS1        (0x08)
#define MQTT_CONNECT_WILL_QoS2        (0x10)
#define MQTT_CONNECT_WILL_RETAIN      (0x20)
#define MQTT_CONNECT_PASSWORD         (0x40)
#define MQTT_CONNECT_USERNAME         (0x80)

/*fixed-header flags*/
#define MQTT_NO_FLAGS                 (0x00)
#define MQTT_FLAG_DUP                 (0x08)
#define MQTT_FLAG_QoS0                (0x00)
#define MQTT_FLAG_QoS1                (0x02)
#define MQTT_FLAG_QoS2                (0x04)
#define MQTT_FLAG_RETAIN              (0x01)

/*connect message constants*/
#define MQTT_CONNECT_M                'M'
#define MQTT_CONNECT_Q                'Q'
#define MQTT_CONNECT_I                'I'
#define MQTT_CONNECT_s                's'
#define MQTT_CONNECT_d                'd'
#define MQTT_CONNECT_p                'p'

typedef struct{
  int sockfd;
  char client_id[23];
  char username[13];
  char password[13];
  uint8_t will_retain;
  uint8_t will_qos;
  uint8_t clean_session;
  uint16_t keep_alive;
  uint8_t debug_session;
}broker_handle_t;

enum ack_sizes{
  CONNACK = 4,
  PUBACK = 4,
  PUBREC = 4,
  PUBCOMP = 4,
  SUBACK = 5,
  UNSUBACK = 4,
  PINGRESP = 2
};

enum connack_return_code{
  CONNECTION_ACCEPTED = (0x00),
  UNPROTOCOL_VERSION = (0x01),
  IDENTIFIER_REJECTED = (0x02),
  SERVER_UNAVAILABLE = (0x03),
  BAD_USER_OR_PASS = (0x04),
  NOT_AUTHORIZED = (0x05)
};

extern uint16_t mqtt_id;
extern uint16_t mqtt_msg_id;
extern uint8_t mqtt_send_buffer[MAX_PACKET_LENGTH];
extern uint8_t mqtt_recv_buffer[MAX_PACKET_LENGTH];
extern time_t last_send;
extern time_t last_receive;

int mqtt_check_msgtype(uint8_t type_id);
int mqtt_create_socket(const char* host, const char* port);
const char* mqtt_get_msgtype(uint8_t type_id);
void mqtt_set_client_id(broker_handle_t *broker, const char *id);
void mqtt_set_userpass(broker_handle_t *broker, const char *username, const char *pass);
int mqtt_init(broker_handle_t* broker, const char* host, const char* port);
void mqtt_destroy(broker_handle_t* broker);
int mqtt_recv_connack(broker_handle_t* broker);
int mqtt_recv_ping(broker_handle_t* broker);
int mqtt_recv_packet(broker_handle_t* broker, size_t recv_buffer_size, uint8_t type_id);
int mqtt_recv_puback(broker_handle_t* broker);
int mqtt_recv_pubrec(broker_handle_t* broker);
int mqtt_recv_pubcomp(broker_handle_t* broker);
int mqtt_recv_suback(broker_handle_t* broker);
int mqtt_recv_unsuback(broker_handle_t* broker);
int mqtt_send_connect(broker_handle_t* broker);
int mqtt_send_disconnect(broker_handle_t* broker);
int mqtt_send_ping(broker_handle_t* broker);
int mqtt_send_packet(broker_handle_t* broker, size_t send_buffer_size, uint8_t type_id);
int mqtt_send_publish(broker_handle_t* broker, const char* topic_name, const char* msg, uint8_t qos, uint8_t dup, uint8_t retain);
int mqtt_send_pubrel(broker_handle_t* broker);
int mqtt_send_subscribe(broker_handle_t* broker, const char* topic_name, uint8_t qos);
int mqtt_send_unsubscribe(broker_handle_t* broker, const char* topic_name);

#endif
