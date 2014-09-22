#ifndef __WPROGRAM_H__
#include <WaspClasses.h>
#endif

#include "WaspMqttSN.h"

WaspMqttSN::WaspMqttSN()
{
  debug = 0;
  clean_session = 1;
  keep_alive = (0x003c);
  id = 0;
  msg_id = 0;
  previous = 0;
  topic_count = 0;
  memset(client_id, 0, sizeof(client_id));
  sprintf(client_id, "mqtt-sn:%d", ++id);
  memset(macHigh, 0, sizeof(macHigh));
  memset(macLow, 0, sizeof(macLow));
}

void WaspMqttSN::set_debug_on() { debug = 1; }

void WaspMqttSN::set_gw_mac(const char *mac) { gw_mac = mac; }

void WaspMqttSN::set_keep_alive(uint16_t duration) { keep_alive = duration; }

void WaspMqttSN::set_client_id(char *my_client_id)
{
  uint8_t clientidlen = strlen(my_client_id);
  memset(client_id, 0, sizeof(client_id));
  memcpy(client_id, my_client_id, clientidlen);
}

void WaspMqttSN::obtain_sensorMac()
{
  xbee802.ON();
  delay(1000);
  xbee802.flush();

  // Get the XBee MAC address
  int counter = 0;
  while((xbee802.getOwnMac()!=0) && (counter<12))
    {
      xbee802.getOwnMac();
      counter++;
    }

  // convert mac address from array to string
  Utils.hex2str(xbee802.sourceMacHigh, macHigh, 4);
  Utils.hex2str(xbee802.sourceMacLow,  macLow,  4);

  delay(1000);
  xbee802.OFF();
}

int WaspMqttSN::check_msgtype(uint8_t type_id)
{
  const char *msg_type = WaspMqttSN::get_msgtype(type_id);
  if(mqttsn_recv_buffer == NULL) {
    USB.printf("mqttsn_check_msgtype(): %s not received\n", msg_type);
    exit(-1);
  }
  else {
    if(mqttsn_recv_buffer[1] == type_id)
      return SUCCESS;
    else {
      if(type_id == 0x1E)
        return SUCCESS;
      else
        return -1;
    }
  }
}

const char* WaspMqttSN::get_msgtype(uint8_t type_id)
{
  switch(type_id) {
  case MQTTSN_TYPE_CONNECT:
    return "CONNECT";
    break;
  case MQTTSN_TYPE_CONNACK:
    return "CONNACK";
    break;
  case MQTTSN_TYPE_REGISTER:
    return "REGISTER";
    break;
  case MQTTSN_TYPE_REGACK:
    return "REGACK";
    break;
  case MQTTSN_TYPE_PUBLISH:
    return "PUBLISH";
    break;
  case MQTTSN_TYPE_PUBACK:
    return "PUBACK";
    break;
  case MQTTSN_TYPE_SUBSCRIBE:
    return "SUBSCRIBE";
    break;
  case MQTTSN_TYPE_SUBACK:
    return "SUBACK";
    break;
  case MQTTSN_TYPE_UNSUBSCRIBE:
    return "UNSUBSCRIBE";
    break;
  case MQTTSN_TYPE_UNSUBACK:
    return "UNSUBACK";
    break;
  case MQTTSN_TYPE_PINGREQ:
    return "PINGREQ";
    break;
  case MQTTSN_TYPE_PINGRESP:
    return "PINGRESP";
    break;
  case MQTTSN_TYPE_DISCONNECT:
    return "DISCONNECT";
    break;
  default:
    return "Unknown type of message";
  }
}

int WaspMqttSN::connect()
{
  uint16_t clientidlen = strlen(client_id), offset=0, flags=0x00;
  uint16_t packetlen = clientidlen+6;

  if(clean_session)
    flags |= MQTTSN_FLAG_CLEAN_SESSION;

  //packet
  mqttsn_send_buffer = (uint8_t*)malloc(packetlen*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, packetlen);

  //header
  mqttsn_send_buffer[0] = packetlen;
  mqttsn_send_buffer[1] = MQTTSN_TYPE_CONNECT;
  //variable-part
  mqttsn_send_buffer[2] = flags;
  mqttsn_send_buffer[3] = MQTTSN_PROTOCOL_VERSION;
  mqttsn_send_buffer[4] = (keep_alive)>>8;
  mqttsn_send_buffer[5] = (keep_alive)&(0xFF);
  memcpy(mqttsn_send_buffer+6, client_id, clientidlen);

  return WaspMqttSN::send_packet(packetlen, MQTTSN_TYPE_CONNECT);
}

int WaspMqttSN::pingreq()
{
  uint8_t packetlen = 2;
  mqttsn_send_buffer = (uint8_t*)malloc(packetlen*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, packetlen);

  //header
  mqttsn_send_buffer[0] = packetlen;
  mqttsn_send_buffer[1] = MQTTSN_TYPE_PINGREQ;

  return WaspMqttSN::send_packet(packetlen, MQTTSN_TYPE_PINGREQ);
}

int WaspMqttSN::pingreq_with_clientID()
{
  uint8_t clientidlen = strlen(client_id), packetlen = 2;
  if(clientidlen)
    packetlen += clientidlen;

  mqttsn_send_buffer = (uint8_t*)malloc(packetlen*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, packetlen);

  //header
  mqttsn_send_buffer[0] = packetlen;
  mqttsn_send_buffer[1] = MQTTSN_TYPE_PINGREQ;
  //variable-part
  memcpy(mqttsn_send_buffer+2, client_id, clientidlen);

  return WaspMqttSN::send_packet(packetlen, MQTTSN_TYPE_PINGREQ);
}

int WaspMqttSN::disconnect()
{
  mqttsn_send_buffer = (uint8_t*)malloc(2*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, 2);

  //header
  mqttsn_send_buffer[0] = (0x02);
  mqttsn_send_buffer[1] = MQTTSN_TYPE_DISCONNECT;

  return WaspMqttSN::send_packet(2, MQTTSN_TYPE_DISCONNECT);
}

int WaspMqttSN::disconnect(uint16_t duration)
{
  mqttsn_send_buffer = (uint8_t*)malloc(4*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, 4);

  //header
  mqttsn_send_buffer[0] = (0x04);
  mqttsn_send_buffer[1] = MQTTSN_TYPE_DISCONNECT;
  //variable-part
  mqttsn_send_buffer[2] = (duration)>>8;
  mqttsn_send_buffer[3] = (duration)&(0xFF);

  return WaspMqttSN::send_packet(4, MQTTSN_TYPE_DISCONNECT);
}

int WaspMqttSN::subscribe_topicId(uint16_t topicId, uint8_t qos, uint8_t topicType)
{
  uint16_t packetlen = 7;

  mqttsn_send_buffer = (uint8_t*)malloc(packetlen*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, packetlen);
  msg_id++;

  //set flags
  uint8_t flags = topicType;
  if(!qos)
    flags |= MQTTSN_FLAG_QoS0;
  else if(qos == 1)
    flags |= MQTTSN_FLAG_QoS1;
  else
    flags |= MQTTSN_FLAG_QoS2;

  //header
  mqttsn_send_buffer[0] = packetlen;
  mqttsn_send_buffer[1] = MQTTSN_TYPE_SUBSCRIBE;
  //variable-part
  mqttsn_send_buffer[2] = flags;
  mqttsn_send_buffer[3] = (msg_id)>>8;
  mqttsn_send_buffer[4] = (msg_id)&(0xFF);
  mqttsn_send_buffer[5] = (topicId)>>8;
  mqttsn_send_buffer[6] = (topicId)&(0xFF);

  return WaspMqttSN::send_packet(7, MQTTSN_TYPE_SUBSCRIBE);
}

int WaspMqttSN::subscribe_topicName(const char* topicName, uint8_t qos)
{
  uint16_t topicnamelen = strlen(topicName);
  uint16_t packetlen = 5+topicnamelen;

  mqttsn_send_buffer = (uint8_t*)malloc(packetlen*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, packetlen);
  msg_id++;

  //set flags
  uint8_t flags = MQTTSN_TOPIC_TYPE_NORMAL;
  if(!qos)
    flags |= MQTTSN_FLAG_QoS0;
  else if(qos == 1)
    flags |= MQTTSN_FLAG_QoS1;
  else
    flags |= MQTTSN_FLAG_QoS2;

  //header
  mqttsn_send_buffer[0] = packetlen;
  mqttsn_send_buffer[1] = MQTTSN_TYPE_SUBSCRIBE;
  //variable-part
  mqttsn_send_buffer[2] = flags;
  mqttsn_send_buffer[3] = (msg_id)>>8;
  mqttsn_send_buffer[4] = (msg_id)&(0xFF);
  memcpy(mqttsn_send_buffer+5, topicName, topicnamelen);

  return WaspMqttSN::send_packet(packetlen, MQTTSN_TYPE_SUBSCRIBE);
}

int WaspMqttSN::unsubscribe_topicId(uint16_t topicId, uint8_t topicType)
{
  uint16_t packetlen = 7;

  mqttsn_send_buffer = (uint8_t*)malloc(packetlen*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, packetlen);
  msg_id++;

  //set flags
  uint8_t flags = topicType;

  //header
  mqttsn_send_buffer[0] = packetlen;
  mqttsn_send_buffer[1] = MQTTSN_TYPE_UNSUBSCRIBE;
  //variable-part
  mqttsn_send_buffer[2] = flags;
  mqttsn_send_buffer[3] = (msg_id)>>8;
  mqttsn_send_buffer[4] = (msg_id)&(0xFF);
  mqttsn_send_buffer[5] = (topicId)>>8;
  mqttsn_send_buffer[6] = (topicId)&(0xFF);

  return WaspMqttSN::send_packet(packetlen, MQTTSN_TYPE_UNSUBSCRIBE);
}

int WaspMqttSN::unsubscribe_topicName(const char* topicName)
{
  uint16_t topicnamelen = strlen(topicName);
  uint16_t packetlen = 5+topicnamelen;

  mqttsn_send_buffer = (uint8_t*)malloc(packetlen*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, packetlen);
  msg_id++;

  //flags
  uint8_t flags = MQTTSN_TOPIC_TYPE_NORMAL;

  //header
  mqttsn_send_buffer[0] = packetlen;
  mqttsn_send_buffer[1] = MQTTSN_TYPE_UNSUBSCRIBE;
  //variable-part
  mqttsn_send_buffer[2] = flags;
  mqttsn_send_buffer[3] = (msg_id)>>8;
  mqttsn_send_buffer[4] = (msg_id)&(0xFF);
  memcpy(mqttsn_send_buffer+5, topicName, topicnamelen);

  return WaspMqttSN::send_packet(packetlen, MQTTSN_TYPE_UNSUBSCRIBE);
}

int WaspMqttSN::publish(uint16_t topicId, uint8_t topicType, const char *msg, uint8_t qos, uint8_t dup, uint8_t retain)
{
  uint16_t msglen = strlen(msg);
  uint16_t packetlen = 7+msglen;

  mqttsn_send_buffer = (uint8_t*)malloc(packetlen*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, packetlen);
  msg_id++;

  //set flags
  uint8_t flags = topicType;
  if(dup)
    flags |= MQTTSN_FLAG_DUP;
  if(retain)
    flags |= MQTTSN_FLAG_RETAIN;
  if(!qos)
    flags |= MQTTSN_FLAG_QoS0;
  else if(qos==1)
    flags |= MQTTSN_FLAG_QoS1;
  else if(qos==2)
    flags |= MQTTSN_FLAG_QoS2;
  else
    flags |= MQTTSN_FLAG_QoSm1;

  //header
  mqttsn_send_buffer[0] = packetlen;
  mqttsn_send_buffer[1] = MQTTSN_TYPE_PUBLISH;
  //variable-part
  mqttsn_send_buffer[2] = flags;
  mqttsn_send_buffer[3] = (topicId)>>8;
  mqttsn_send_buffer[4] = (topicId)&(0xFF);
  mqttsn_send_buffer[5] = (msg_id)>>8;
  mqttsn_send_buffer[6] = (msg_id)&(0xFF);
  memcpy(mqttsn_send_buffer+7, msg, msglen);

  return WaspMqttSN::send_packet(packetlen, MQTTSN_TYPE_PUBLISH);
}

int WaspMqttSN::pubrel()
{
  uint16_t packetlen = 4;

  mqttsn_send_buffer = (uint8_t*)malloc(packetlen*sizeof(uint8_t));
  memset(mqttsn_send_buffer, 0, packetlen);
  msg_id++;

  //header
  mqttsn_send_buffer[0] = packetlen;
  mqttsn_send_buffer[1] = MQTTSN_TYPE_PUBREL;
  //variable-part
  mqttsn_send_buffer[2] = (msg_id)>>8;
  mqttsn_send_buffer[3] = (msg_id)&(0xFF);

  return WaspMqttSN::send_packet(packetlen, MQTTSN_TYPE_PUBREL);
}

int WaspMqttSN::register_topicName(const char *topicName)
{
  int ret = 0;
  if(topic_count < (MAX_TOPICS-1))
    {
      uint16_t topicnamelen = strlen(topicName);
      uint16_t packetlen = topicnamelen+6;

      mqttsn_send_buffer = (uint8_t*)malloc(packetlen*sizeof(uint8_t));
      memset(mqttsn_send_buffer, 0, packetlen);
      msg_id++;

      //store topic name
      topic_table[topic_count].name = topicName;
      topic_table[topic_count].id = 0;

      //header
      mqttsn_send_buffer[0] = packetlen;
      mqttsn_send_buffer[1] = MQTTSN_TYPE_REGISTER;
      //variable-part
      mqttsn_send_buffer[2] = 0x00;
      mqttsn_send_buffer[3] = 0x00;
      mqttsn_send_buffer[4] = (msg_id)>>8;
      mqttsn_send_buffer[5] = (msg_id)&(0xFF);
      memcpy(mqttsn_send_buffer+6, topicName, topicnamelen);

      ret = WaspMqttSN::send_packet(packetlen, MQTTSN_TYPE_REGISTER);
    }
  else {
    fprintf(stderr, "register_topicName(): Error, no free space to store more topics.\n");
    ret = -2;
  }
  return ret;
}

int WaspMqttSN::recv_connack()
{
  int ret = WaspMqttSN::recv_packet(CONNACK_SIZE, MQTTSN_TYPE_CONNACK);
  if(ret > 0) {
    if(mqttsn_recv_buffer[2] == ACCEPTED)
      ret = 1;
    else
      ret = -1;
  }

  free(mqttsn_recv_buffer);
  mqttsn_recv_buffer = NULL;

  return ret;
}

int WaspMqttSN::recv_suback()
{
  int ret = WaspMqttSN::recv_packet(SUBACK_SIZE, MQTTSN_TYPE_SUBACK);
  if(ret > 0) {
    if( (msg_id>>8 != mqttsn_recv_buffer[5]) || ((msg_id&(0xFF)) != mqttsn_recv_buffer[6]) ) {
      if(debug)
	USB.println("recv_suback(): Wrong message id");
      ret = -1;
    }
    else {
      if(mqttsn_recv_buffer[7] == ACCEPTED)
	if(debug)
	  USB.printf("Suback received, granted QoS of %d", (mqttsn_recv_buffer[2]&(0x60))>>5);
      ret = 1;
    }
  }

  free(mqttsn_recv_buffer);
  mqttsn_recv_buffer = NULL;

  return ret;
}

int WaspMqttSN::recv_unsuback()
{
  int ret = WaspMqttSN::recv_packet(UNSUBACK_SIZE, MQTTSN_TYPE_UNSUBACK);
  if(ret > 0) {
    if( (msg_id>>8 != mqttsn_recv_buffer[2]) || ((msg_id&(0xFF)) != mqttsn_recv_buffer[3]) ) {
      if(debug)
	USB.println("recv_unsuback(): Wrong message id");
      ret = -1;
    }
  }

  free(mqttsn_recv_buffer);
  mqttsn_recv_buffer=NULL;

  return ret;
}

int WaspMqttSN::recv_puback()
{
  int ret = WaspMqttSN::recv_packet(PUBACK_SIZE, MQTTSN_TYPE_PUBACK);
  if(ret > 0) {
    if( (msg_id>>8 != mqttsn_recv_buffer[4]) || ((msg_id&(0xFF)) != mqttsn_recv_buffer[5]) ) {
      if(debug)
	USB.println("recv_puback(): Wrong message id");
      ret = -1;
    }
    else {
      if(mqttsn_recv_buffer[6] == ACCEPTED)
	if(debug)
	  USB.printf("Puback received");
      ret = 1;
    }
  }

  free(mqttsn_recv_buffer);
  mqttsn_recv_buffer=NULL;

  return ret;
}

int WaspMqttSN::recv_pubrec()
{
  int ret = WaspMqttSN::recv_packet(PUBREC_SIZE, MQTTSN_TYPE_PUBREC);
  if(ret > 0) {
    if( (msg_id>>8 != mqttsn_recv_buffer[2]) || ((msg_id&(0xFF)) != mqttsn_recv_buffer[3]) ) {
      if(debug)
	USB.println("recv_unsuback(): Wrong message id");
      ret = -1;
    }
  }

  free(mqttsn_recv_buffer);
  mqttsn_recv_buffer=NULL;

  return ret;
}

int WaspMqttSN::recv_pubcomp()
{
  int ret = WaspMqttSN::recv_packet(PUBCOMP_SIZE, MQTTSN_TYPE_PUBCOMP);
  if(ret > 0) {
    if( (msg_id>>8 != mqttsn_recv_buffer[2]) || ((msg_id&(0xFF)) != mqttsn_recv_buffer[3]) ) {
      if(debug)
	USB.println("recv_pubcomp(): Wrong message id");
      ret = -1;
    }
  }

  free(mqttsn_recv_buffer);
  mqttsn_recv_buffer=NULL;

  return ret;
}

int WaspMqttSN::recv_regack()
{
  int ret = WaspMqttSN::recv_packet(REGACK_SIZE, MQTTSN_TYPE_REGACK);
  if(ret > 0) {
    if( (msg_id>>8 != mqttsn_recv_buffer[4]) || ((msg_id&(0xFF)) != mqttsn_recv_buffer[5]) ) {
      if(debug)
	USB.println("recv_regack(): Wrong message id.");
      ret = -1;
    }
    else if(mqttsn_recv_buffer[6] == REJECTED_INVALID_TOPIC_ID) {
      USB.println("recv_regack(): Error, rejected invalid topic id.\n");
      ret = -2;
    }
    else if(mqttsn_recv_buffer[6] == ACCEPTED) {
      uint16_t idMSB = (uint16_t)mqttsn_recv_buffer[2];
      uint16_t idLSB = (uint16_t)mqttsn_recv_buffer[3];
      topic_table[topic_count].id = (idMSB<<8)|(idLSB);
      topic_count++;
    }
  }

  free(mqttsn_recv_buffer);
  mqttsn_recv_buffer=NULL;

  return ret;
}

int WaspMqttSN::recv_ping()
{
  int ret = WaspMqttSN::recv_packet(PING_SIZE, MQTTSN_TYPE_PINGRESP);
  if(ret > 0) {
    ret = 1;
  }

  free(mqttsn_recv_buffer);
  mqttsn_recv_buffer=NULL;

  return ret;
}

uint16_t WaspMqttSN::look_up_topicId(const char* topicName)
{
  uint16_t id;
  for(int i=0; i<MAX_TOPICS; i++) {
    if(strcmp(topic_table[i].name, topicName) > 0)
      id=topic_table[i].id;
  }
  return id;
}

int WaspMqttSN::send_packet(uint8_t mqttsn_packet_size, uint8_t type_id)
{
  int ret = 0;
  char mac_tag[] = "#-MAC:";
  char mqtt_tag[] = "#-MQTT:";
  uint8_t end_line = 0x0a;
  packetXBee* packet;

  //turn on xbee and usb if debug
  if(debug)
    USB.ON();

  // generate the buffer to send, with the tags and the mqtt info
  uint8_t buffer[MAX_PACKET_LENGTH];
  int offset = 0;

  memset(buffer, 0, sizeof(buffer));
  //mac-tag
  memcpy(buffer, mac_tag, sizeof(mac_tag));
  offset += sizeof(mac_tag);
  //macHigh and macLow
  memcpy(buffer+offset, macHigh, sizeof(macHigh));
  offset += sizeof(macHigh);
  memcpy(buffer+offset, macLow, sizeof(macLow));
  offset += sizeof(macLow);
  //mqtt-tag
  memcpy(buffer+offset, mqtt_tag, sizeof(mqtt_tag));
  offset += sizeof(mqtt_tag);
  //mqttsn_packet
  memcpy(buffer+offset, mqttsn_send_buffer, mqttsn_packet_size);
  offset += mqttsn_packet_size;
  //put the end line caracter
  memcpy(buffer+offset, &end_line, 1);
  offset++;

  //send the buffer
  packet = (packetXBee*)calloc(1, sizeof(packetXBee)); // Memory allocation
  packet->mode = UNICAST; // Choose transmission mode: UNICAST or BROADCAST

  xbee802.setDestinationParams(packet, gw_mac, buffer, offset, MAC_TYPE);
  xbee802.sendXBee(packet);

  if(debug)
    USB.printf("send %s\n", WaspMqttSN::get_msgtype(type_id));

  // check TX flag
  if( xbee802.error_TX == 0 ) {
    ret = 1;
    if(debug)
      USB.printf("OK\n");
  }
  else {
    ret = -1;
    if(debug)
      USB.printf("FAIL\n");
  }

  // free variables
  free(packet);
  packet=NULL;
  free(mqttsn_send_buffer);
  mqttsn_send_buffer=NULL;

  //turn off
  if(debug)
    USB.OFF();

  return ret;
}

int WaspMqttSN::recv_packet(uint8_t mqttsn_packet_size, uint8_t type_id)
{
  int bufferlen = 0, error = 0;

  if(debug)
    USB.ON();

  previous = millis();
  while( (millis()-previous) < 5000 ) {
    if( xbee802.available() > 0 )
      {
	// treat available bytes in order to parse the information as XBee packets
	xbee802.treatData();
	// check RX flag after 'treatData'
	if( !xbee802.error_RX )
	  {
	    // read available packets
	    while( xbee802.pos>0 )
	      {
		// Available information in 'xbee802.packet_finished' structure
		int data_recv = xbee802.packet_finished[xbee802.pos-1]->data_length;

		if(data_recv > 0)
		  {
		    bufferlen = data_recv - 13;
		    if(bufferlen != mqttsn_packet_size) {
		      if(debug)
			USB.printf("Unexpected packet size %d - %d\n", bufferlen, mqttsn_packet_size);
		      error = 1;
		    }
		    else {
		      char *offset = xbee802.packet_finished[xbee802.pos-1]->data;
		      mqttsn_recv_buffer = (uint8_t*)malloc(bufferlen*sizeof(uint8_t));
		      memset(mqttsn_recv_buffer, 0, bufferlen);
		      memcpy(mqttsn_recv_buffer, offset+13, bufferlen);

		      //restore 0x00 characters
		      for(int x=0; x<bufferlen; x++) {
			if(mqttsn_recv_buffer[x] == (0x23))
			  mqttsn_recv_buffer[x] = 0x00;
		      }

		      if(WaspMqttSN::check_msgtype(type_id) != -1) {
			if(debug)
			  USB.printf("Received %s\n", WaspMqttSN::get_msgtype(type_id));
		      }
		      else {
			USB.printf("%s not received\n", WaspMqttSN::get_msgtype(type_id));
			error = 1;
		      }
		    }
		  }
		else
		  {
		    fprintf(stderr, "mqttsn_recv_packet(): %s\n", errno);
		    error = 1;
		  }

		if(!error) {
		  if(debug) {
		    USB.println("recv_packet():");
		    USB.printf("-- %d data received\n", bufferlen);
		    USB.print(F("-- Data: "));
		    for(int i=0;i<bufferlen;i++)
		      {
			USB.printf("%x, ", mqttsn_recv_buffer[i]);
		      }
		    USB.println("");
		  }
		}
		else
		  bufferlen = -1;

		// free memory
		free(xbee802.packet_finished[xbee802.pos-1]);
		// free pointer
		xbee802.packet_finished[xbee802.pos-1]=NULL;
		// decrement the received packet counter
		xbee802.pos--;
	      }
	    previous = millis();
	  }
      }
    // Condition to avoid an overflow (DO NOT REMOVE)
    if (millis() < previous) {
      previous = millis();
    }
  }

  if(debug)
    USB.OFF();

  return bufferlen;
}



WaspMqttSN mqttSN = WaspMqttSN();
