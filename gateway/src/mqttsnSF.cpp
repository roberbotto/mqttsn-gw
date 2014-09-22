#include "mqttsnSF.h"
#include <map>
#include <iostream>

using namespace std;

uint16_t msgid = 0;
uint16_t topic_id_count = 0;

typedef std::map<int, std::string> localmap;
typedef std::map<std::string, localmap> globalmap;

localmap default_localMap;
globalmap g_topicMap;

int mqttsn2mqtt(char *mqttsn, broker_handle_t *broker, int msgtype, char *mac)
{
  int res;
  char *offset;

  switch(msgtype)
    {

    case MQTTSN_TYPE_CONNECT:
      {
	uint16_t durationB1 = (uint16_t)mqttsn[4];
	uint16_t durationB2 = (uint16_t)mqttsn[5];
	broker->clean_session = (mqttsn[2] & MQTTSN_FLAG_CLEAN_SESSION)>>2;
	broker->keep_alive = (durationB1<<8)|(durationB2);
	res = mqtt_send_connect(broker);
	break;
      }

    case MQTTSN_TYPE_DISCONNECT:
      {
	res = mqtt_send_disconnect(broker);
	close(broker->sockfd);
	break;
      }

    case MQTTSN_TYPE_SUBSCRIBE:
      {
	char *topicName;

	//set qos
	uint8_t qos = mqttsn[2]&(0x60);
	if(qos == MQTTSN_FLAG_QoS0)        qos = 0;
	else if(qos == MQTTSN_FLAG_QoS1)   qos = 1;
	else if(qos == MQTTSN_FLAG_QoS2)   qos = 2;

	//get topic name
	topicName = process_topicIdType(mqttsn, mac, msgtype);
	//send subscribe
	res = mqtt_send_subscribe(broker, topicName, qos);

	free(topicName);
	topicName=NULL;
	break;
      }

    case MQTTSN_TYPE_UNSUBSCRIBE:
      {
	char *topicName;

	//get topic name
	topicName = process_topicIdType(mqttsn, mac, msgtype);
	//send unsubscribe
	res = mqtt_send_unsubscribe(broker, topicName);

	free(topicName);
	topicName=NULL;
	break;
      }

    case MQTTSN_TYPE_PUBLISH:
      {
	offset = mqttsn;
	uint8_t qos = mqttsn[2]&(0x60);
	uint8_t dup = (mqttsn[2]&(MQTTSN_FLAG_DUP))>>8;
	uint8_t retain = (mqttsn[2]&(MQTTSN_FLAG_RETAIN))>>4;
	char *topicName;

	//obtain the msg
	int msglen = mqttsn[0] - 7;
	char *msg = (char*)malloc((msglen+1)*sizeof(char));
	memset(msg, 0, msglen);
	memcpy(msg, offset+7, msglen);
	msg[msglen] = '\0';

	topicName = process_topicIdType(mqttsn, mac, msgtype);

	if(qos == MQTTSN_FLAG_QoSm1) {
	  qos = 0;
	  //inicializar conexion MQTT
	  res = mqtt_send_connect(broker);
	  res = mqtt_recv_connack(broker);
	  //send publish
	  res = mqtt_send_publish(broker, topicName, msg, qos, dup, retain);
	  //disconnect and close
	  res = mqtt_send_disconnect(broker);
	  close(broker->sockfd);
	}
	else {
	  if(qos == MQTTSN_FLAG_QoS0)        qos = 0;
	  else if(qos == MQTTSN_FLAG_QoS1)   qos = 1;
	  else if(qos == MQTTSN_FLAG_QoS2)   qos = 2;
	  //only send publish
	  res = mqtt_send_publish(broker, topicName, msg, qos, dup, retain);
	}

	free(msg);
	msg=NULL;
	break;
      }

    case MQTTSN_TYPE_PUBREL:
      {
	res = mqtt_send_pubrel(broker);
	break;
      }

    case MQTTSN_TYPE_PINGREQ:
      {
	res = mqtt_send_ping(broker);
	break;
      }

    default:
      fprintf(stderr, "wrong msgtype: %x\n", msgtype);
    }

  return res;
}

void mqtt2mqttsn(const char *mac)
{
  int i, len, bufAllocated = 0;
  char msgtype = (mqtt_recv_buffer[0] & 0xF0);
  char *buf;

  switch(msgtype)
    {

    case MQTT_TYPE_CONNACK:
      {
	fprintf(stderr, "---Connack\n");
	buf = (char*)malloc(CONNACK_SIZE*sizeof(char));
	buf[0] = 0x33;
	buf[1] = MQTTSN_TYPE_CONNACK;
	if(mqtt_recv_buffer[3] == 0x00)
	  buf[2] = ACCEPTED;
	else
	  buf[2] = REJECTED_CONGESTION;

	len = CONNACK_SIZE;
	bufAllocated = 1;
	break;
      }

    case MQTT_TYPE_SUBACK:
      {
	fprintf(stderr, "---Suback\n");
	//set flags
	char flags = (0x00);
	int mqtt_qos = mqtt_recv_buffer[4]&(0x03);
	if(mqtt_qos == 0x00)
	  flags |= MQTTSN_FLAG_QoS0;
	else if(mqtt_qos == 0x01)
	  flags |= MQTTSN_FLAG_QoS1;
	else if(mqtt_qos == 0x02)
	  flags |= MQTTSN_FLAG_QoS2;

	buf = (char*)malloc(SUBACK_SIZE*sizeof(char));
	buf[0] = 0x38;
	buf[1] = MQTTSN_TYPE_SUBACK;
	buf[2] = flags;
	buf[3] = 0x1e;
	buf[4] = 0x1e;
	buf[5] = mqtt_recv_buffer[2];
	buf[6] = mqtt_recv_buffer[3];
	buf[7] = ACCEPTED;

	len = SUBACK_SIZE;
	bufAllocated = 1;
	break;
      }

    case MQTT_TYPE_UNSUBACK:
      {
	fprintf(stderr, "---Unsuback\n");
	buf = (char*)malloc(UNSUBACK_SIZE*sizeof(char));
	buf[0] = UNSUBACK_SIZE;
	buf[1] = MQTTSN_TYPE_UNSUBACK;
	buf[2] = mqtt_recv_buffer[3];
	buf[3] = mqtt_recv_buffer[4];

	len = UNSUBACK_SIZE;
	bufAllocated = 1;
	break;
      }

    case MQTT_TYPE_PUBACK:
      {
	fprintf(stderr, "---Puback\n");
	buf = (char*)malloc(PUBACK_SIZE*sizeof(char));
	buf[0] = 0x37;
	buf[1] = MQTTSN_TYPE_PUBACK;
	buf[2] = 0x1e;
	buf[3] = 0x1e; //cambiar estos valores por el topicId del publish anterior
	buf[4] = mqtt_recv_buffer[2];
	buf[5] = mqtt_recv_buffer[3];
	buf[6] = ACCEPTED;

	len = PUBACK_SIZE;
	bufAllocated = 1;
	break;
      }

    case MQTT_TYPE_PINGRESP:
      {
	buf = (char*)malloc(PING_SIZE*sizeof(char));
	buf[0] = 0x32;
	buf[1] = MQTTSN_TYPE_PINGRESP;

	len = PING_SIZE;
	bufAllocated = 1;
	break;
      }

    case MQTT_TYPE_PUBREC:
      {
	buf = (char*)malloc(PUBREC_SIZE*sizeof(char));
	buf[0] = 0x34;
	buf[1] = MQTTSN_TYPE_PUBREC;
	buf[2] = mqtt_recv_buffer[2];
	buf[3] = mqtt_recv_buffer[3];

	len = PUBREC_SIZE;
	bufAllocated = 1;
	break;
      }

    case MQTT_TYPE_PUBCOMP:
      {
	buf = (char*)malloc(PUBCOMP_SIZE*sizeof(char));
	buf[0] = 0x34;
	buf[1] = MQTTSN_TYPE_PUBCOMP;
	buf[2] = mqtt_recv_buffer[2];
	buf[3] = mqtt_recv_buffer[3];

	len = PUBCOMP_SIZE;
	bufAllocated = 1;
	break;
      }

    default:
      fprintf(stderr, "mqtt2mqttsn(): Error, wrong message type: %d\n", msgtype);
    }

  if(bufAllocated) {
    //replace each 0x00 by 0x23
    for(i=0; i<len; i++) {
      if(buf[i]==0x00)
	buf[i] = 0x23;
    }

    send_byMac(mac, buf);

    free(buf);
    buf=NULL;
  }
}

void register_handler(char *mqttsn, char *mac)
{
  char *offset = mqttsn;
  uint8_t retCode = REJECTED_INVALID_TOPIC_ID;
  uint8_t topic_id;

  //obtain topic name
  int topicnamelen = mqttsn[0] - 6;
  char *topicName = (char*)malloc((topicnamelen+1)*sizeof(char));
  memset(topicName, 0, topicnamelen);
  memcpy(topicName, offset+6, topicnamelen);
  topicName[topicnamelen] = '\0';

  topic_id = store_topicId(topicName, mac);

  if(topic_id != -1) {
    retCode = ACCEPTED;
    topic_id_count++;
  }

  free(topicName);
  topicName = NULL;

  send_regack(topic_id, mqttsn[4], mqttsn[5], retCode, mac);
}

uint8_t store_topicId(char *topicName, char *mac)
{
  uint8_t ret;
  std::string mac_str(mac,16);
  std::map<string, localmap>::const_iterator search = g_topicMap.find(mac_str);

  if(search != g_topicMap.end()) {
    localmap aux = search->second;
    std::map<int, string>::const_iterator s = aux.find(topic_id_count);
    if(s != aux.end()) {
      ret = -1;
    } else {
      aux[topic_id_count] = topicName;
      g_topicMap[mac_str] = aux;
      ret = topic_id_count;
    }
  }
  else {
    localmap l_topicMap;
    l_topicMap.insert(std::make_pair(topic_id_count, topicName));
    g_topicMap.insert(std::make_pair(mac_str, l_topicMap));
    ret = topic_id_count;
  }

  return ret;
}

void send_regack(uint16_t topic_id, uint8_t msg_idMSB, uint8_t msg_idLSB, uint8_t return_code, const char *mac)
{
  char buf[7];
  buf[0] = 0x37;
  buf[1] = MQTTSN_TYPE_REGACK;
  buf[2] = topic_id>>8;
  buf[3] = topic_id&(0xFF);
  buf[4] = msg_idMSB;
  buf[5] = msg_idLSB;
  buf[6] = return_code;

  send_byMac(mac, buf);
}

char* process_topicIdType(char *mqttsn, char *mac, uint8_t msgtype)
{
  char *buf, *offset = mqttsn;
  uint16_t topicIdType = mqttsn[2]&(0x03);

  if(msgtype == MQTTSN_TYPE_SUBSCRIBE || msgtype == MQTTSN_TYPE_UNSUBSCRIBE)
    {
      uint16_t tidMSB = (uint16_t)mqttsn[5];
      uint16_t tidLSB = (uint16_t)mqttsn[6];
      uint16_t topicID = tidMSB<<8 | tidLSB;

      if(topicIdType == MQTTSN_TOPIC_TYPE_NORMAL) {
	//treat of normal topics names
	uint16_t topicnamelen = mqttsn[0] - 5;
	buf = (char*)malloc((topicnamelen+1)*sizeof(char));
	memset(buf, 0, topicnamelen);
	memcpy(buf, offset+5, topicnamelen);
	buf[topicnamelen] = '\0';
      }
      else if(topicIdType == MQTTSN_TOPIC_TYPE_PREDEFINED) {
	buf = getTopicName_byID(mac, topicID);
      }
      else if(topicIdType == MQTTSN_TOPIC_TYPE_SHORT) {
	buf = (char*)malloc(3*sizeof(char));
	memset(buf, 0, 3);
	memcpy(buf, offset+5, 2);
	buf[2] = '\0';
      }
    }
  else if(msgtype == MQTTSN_TYPE_PUBLISH)
    {
      uint16_t tidMSB = (uint16_t)mqttsn[3];
      uint16_t tidLSB = (uint16_t)mqttsn[4];
      uint16_t topicID = tidMSB<<8 | tidLSB;

      if(topicIdType == MQTTSN_TOPIC_TYPE_PREDEFINED) {
	buf = getTopicName_byID(mac, topicID);
      }
      else if(topicIdType == MQTTSN_TOPIC_TYPE_SHORT) {
	buf = (char*)malloc(3*sizeof(char));
	memset(buf, 0, 3);
	memcpy(buf, offset+3, 2);
	buf[2] = '\0';
      }
    }
  else {
    fprintf(stderr, "proccess_topicIdType(): Error, this type of msg can not be processed.\n");
    buf=NULL;
  }

  return buf;
}

char* getTopicName_byID(char *mac, uint16_t topicID)
{
  std::string mac_str(mac,16);
  std::string topicName;
  char * ret_topicName;
  std::map<std::string, localmap>::const_iterator search;
  std::map<int, std::string>::const_iterator search2;

  search = g_topicMap.find(mac_str);
  if(search != g_topicMap.end()) {
    localmap aux = search->second;
    search2 = aux.find(topicID);
    if(search2 != aux.end()) {
      topicName = search2->second;
    } else
      topicName = "";
  }
  else
    fprintf(stderr, "getTopicName_byID(): Error, mac not found.\n");

  if(topicName.size() > 0) {
    ret_topicName = new char[topicName.size()+1];
    std::copy(topicName.begin(), topicName.end(), ret_topicName);
    ret_topicName[topicName.size()] = '\0';
  }

  return ret_topicName;
}

void initTopics_byDefault(char *mac)
{
  std::string mac_str(mac,16);
  default_localMap[0] = std::string("a/b");
  default_localMap[1] = std::string("a/c");
  default_localMap[2] = std::string("a/d");

  g_topicMap.insert(std::make_pair(mac_str, default_localMap));
}
