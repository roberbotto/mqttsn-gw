#include "mqtt.h"

#ifndef AI_DEFAULT
#define AI_DEFAULT (AI_ADDRCONFIG|AI_V4MAPPED)
#endif

uint16_t mqtt_id = 0;
uint16_t mqtt_msg_id = 0x0000;
uint8_t mqtt_send_buffer[MAX_PACKET_LENGTH];
uint8_t mqtt_recv_buffer[MAX_PACKET_LENGTH];
time_t last_send = 0;
time_t last_receive = 0;

int mqtt_check_msgtype(uint8_t type_id)
{
  const char *msg_type = mqtt_get_msgtype(type_id);
  if(mqtt_recv_buffer == NULL) {
    fprintf(stderr, "mqtt_check_msgtype(): %s not received\n", msg_type);
    exit(EXIT_FAILURE);
  }
  else {
    if(mqtt_recv_buffer[0] == type_id)
      return SUCCESS;
    else {
      if(type_id == 0x1E) // special type to use in translation process
        return SUCCESS;
      else
        return -1;
    }
  }
}

int mqtt_create_socket(const char *host, const char *port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  struct timeval tv;
  int fd, ret;

  // Set options for the resolver
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM; /* Stream socket */
  hints.ai_flags = AI_DEFAULT;    /* Default flags */
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // Lookup address
  ret = getaddrinfo(host, port, &hints, &result);
  if (ret != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
    exit(EXIT_FAILURE);
  }

  /* getaddrinfo() returns a list of address structures.
     Try each address until we successfully connect(2).
     If socket(2) (or connect(2)) fails, we (close the socket and)
     try the next address. */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
      fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (fd == -1)
        continue;
      // Connect socket to the remote host
      if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0)
        break;      // Success
      close(fd);
  }

  if (rp == NULL) {
    fprintf(stderr, "Could not connect to remote host.\n");
    exit(EXIT_FAILURE);
  }
  freeaddrinfo(result);

  // FIXME: set the Don't Fragment flag

  // Setup timeout on the socket
  tv.tv_sec = 10;
  tv.tv_usec = 0;
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    perror("Error setting timeout on socket");
  }
  return fd;
}

const char* mqtt_get_msgtype(uint8_t type_id)
{
  switch(type_id)
    {
    case MQTT_TYPE_CONNECT:
      return "CONNECT";
      break;
    case MQTT_TYPE_CONNACK:
      return "CONNACK";
      break;
    case MQTT_TYPE_PUBLISH:
      return "PUBLISH";
      break;
    case MQTT_TYPE_PUBACK:
      return "PUBACK";
      break;
    case MQTT_TYPE_PUBREC:
      return "PUBREC";
      break;
    case MQTT_TYPE_PUBREL:
      return "PUBREL";
      break;
    case MQTT_TYPE_PUBCOMP:
      return "PUBCOMP";
      break;
    case MQTT_TYPE_SUBSCRIBE:
      return "SUBSCRIBE";
      break;
    case MQTT_TYPE_SUBACK:
      return "SUBACK";
      break;
    case MQTT_TYPE_UNSUBSCRIBE:
      return "UNSUBSCRIBE";
      break;
    case MQTT_TYPE_UNSUBACK:
      return "UNSUBACK";
      break;
    case MQTT_TYPE_PINGREQ:
      return "PINGREQ";
      break;
    case MQTT_TYPE_PINGRESP:
      return "PINGRESP";
      break;
    case MQTT_TYPE_DISCONNECT:
      return "DISCONNECT";
      break;
    default:
      return "Unknown type of message";
    }
}

int mqtt_init(broker_handle_t *broker, const char *host, const char *port)
{
  mqtt_id++;
  broker->sockfd = mqtt_create_socket(host,port);
  broker->will_retain = 0;
  broker->will_qos = 0;
  broker->clean_session = 1;
  broker->keep_alive = 180;
  broker->debug_session = 0;

  memset(broker->username, 0, sizeof(broker->username));
  memset(broker->password, 0, sizeof(broker->password));
  memset(broker->client_id, 0, sizeof(broker->client_id));
  sprintf(broker->client_id, "mqtt:%d", mqtt_id);

  return broker->sockfd;
}

void mqtt_destroy(broker_handle_t* broker)
{
  close(broker->sockfd);
  free(broker);
  broker = NULL;
}

void mqtt_set_client_id(broker_handle_t *broker, const char *id)
{
  size_t idlen = sizeof(id);
  memcpy(broker->client_id, id, idlen);
}

void mqtt_set_userpass(broker_handle_t *broker, const char *username, const char *pass)
{
  size_t usernamelen = sizeof(username);
  size_t passlen = sizeof(pass);
  memcpy(broker->username, username, usernamelen);
  memcpy(broker->password, pass, passlen);
}

int mqtt_recv_connack(broker_handle_t *broker)
{
  int ret;

  ret = mqtt_recv_packet(broker, CONNACK, MQTT_TYPE_CONNACK);
  if(ret > 0)
    {
      if(mqtt_recv_buffer[3] == CONNECTION_ACCEPTED) {
        if(broker->debug_session)
          printf("Conexion accepted\n");
          return SUCCESS;
      }
      else {
	switch(mqtt_recv_buffer[3])
	  {
	  case UNPROTOCOL_VERSION:
	    fprintf(stderr, "mqtt_recv_connack(): Unnaceptable protocol version\n");
	    break;
	  case IDENTIFIER_REJECTED:
	    fprintf(stderr, "mqtt_recv_connack(): Identifier rejected\n");
	    break;
	  case SERVER_UNAVAILABLE:
	    fprintf(stderr, "mqtt_recv_connack(): Server unavailable\n");
	    break;
	  case BAD_USER_OR_PASS:
	    fprintf(stderr, "mqtt_recv_connack(): Bad user name or password\n");
	    break;
	  case NOT_AUTHORIZED:
	    fprintf(stderr, "mqtt_recv_connack(): Not authorized\n");
	    break;
	  }
	return -1;
      }
    }
  else return -1;
}

int mqtt_recv_ping(broker_handle_t *broker)
{
  return mqtt_recv_packet(broker, PINGRESP, MQTT_TYPE_PINGRESP);
}

int mqtt_recv_packet(broker_handle_t *broker, size_t recv_buffer_size, uint8_t type_id)
{
  size_t data_recv;
  memset(mqtt_recv_buffer, 0, MAX_PACKET_LENGTH);

  data_recv = recv(broker->sockfd, mqtt_recv_buffer, recv_buffer_size, 0);
  if(data_recv > 0)
    {
      if(mqtt_check_msgtype(type_id) != -1) {
        if(broker->debug_session)
          printf("Received %s\n", mqtt_get_msgtype(type_id));
        return data_recv;
      }
      else {
        printf("%s not received\n", mqtt_get_msgtype(type_id));
        return -1;
      }
    }
  else
    {
      fprintf(stderr, "mqtt_recv_packet(): %s\n", strerror(errno));
      return -1;
    }

  last_receive = time(NULL);

}

int mqtt_recv_puback(broker_handle_t *broker)
{
  int ret;

  ret = mqtt_recv_packet(broker, PUBACK, MQTT_TYPE_PUBACK);
  if(ret > 0)
    {
      if( (mqtt_msg_id>>8 != mqtt_recv_buffer[2]) || ((mqtt_msg_id&(0xFF)) != mqtt_recv_buffer[3]) ) {
	fprintf(stderr, "mqtt_recv_puback(): Wrong message id\n");
	return -1;
      }
      else return SUCCESS;
    }
  else return -1;
}

int mqtt_recv_pubrec(broker_handle_t *broker)
{
  int ret;

  ret = mqtt_recv_packet(broker, PUBREC, MQTT_TYPE_PUBREC);
  if(ret > 0)
    {
      if( (mqtt_msg_id>>8 != mqtt_recv_buffer[2]) || ((mqtt_msg_id&(0xFF)) != mqtt_recv_buffer[3]) ) {
	fprintf(stderr, "mqtt_recv_pubrec(): Wrong message id\n");
	return -1;
      }
      else return SUCCESS;
    }
  else return -1;
}

int mqtt_recv_pubcomp(broker_handle_t *broker)
{
  int ret;

  ret = mqtt_recv_packet(broker, PUBCOMP, MQTT_TYPE_PUBCOMP);
  if(ret > 0)
    {
      if( (mqtt_msg_id>>8 != mqtt_recv_buffer[2]) || ((mqtt_msg_id&(0xFF)) != mqtt_recv_buffer[3]) ) {
	fprintf(stderr, "mqtt_recv_pubcomp(): Wrong message id\n");
	return -1;
      }
      else return SUCCESS;
    }
  else return -1;
}

int mqtt_recv_suback(broker_handle_t *broker)
{
  int ret;

  ret = mqtt_recv_packet(broker, SUBACK, MQTT_TYPE_SUBACK);
  if(ret > 0)
    {
      if( (mqtt_msg_id>>8 != mqtt_recv_buffer[2]) || ((mqtt_msg_id&(0xFF)) != mqtt_recv_buffer[3]) ) {
	fprintf(stderr, "mqtt_recv_suback(): Wrong message id\n");
	return -1;
      }
      else return SUCCESS;
    }
  else return -1;
}

int mqtt_recv_unsuback(broker_handle_t *broker)
{
  int ret;

  ret = mqtt_recv_packet(broker, UNSUBACK, MQTT_TYPE_UNSUBACK);
  if(ret > 0)
    {
      if( (mqtt_msg_id>>8 != mqtt_recv_buffer[2]) || ((mqtt_msg_id&(0xFF)) != mqtt_recv_buffer[3]) ) {
	fprintf(stderr, "mqtt_recv_unsuback(): Wrong message id\n");
	return -1;
      }
      else return SUCCESS;
    }
  else return -1;
}

int mqtt_send_connect(broker_handle_t *broker)
{
  uint16_t clientidlen = strlen(broker->client_id);
  uint16_t usernamelen = strlen(broker->username);
  uint16_t passwordlen = strlen(broker->password);
  uint16_t payloadlen = clientidlen+2;
  uint8_t flags = (0x00), offset = 0;
  uint8_t *payload, var_header[12];
  uint16_t packetlen;

  //set clean-session flag
  if(broker->clean_session) flags |= MQTT_CONNECT_CLEAN_SESSION;

  /* generate payload and update flags */
  payload = (uint8_t*)malloc(payloadlen*sizeof(uint8_t));
  memset(payload,0, payloadlen);

  //set client id
  payload[offset++] = clientidlen >> 8;      //string length MSB
  payload[offset++] = clientidlen & (0xFF);  //string length LSB
  memcpy(payload+offset, broker->client_id, clientidlen);  //encoded character data

  //set username
  if(usernamelen)
    {
      payloadlen += usernamelen+2;
      offset += clientidlen;
      flags |= MQTT_CONNECT_USERNAME;
      payload = (uint8_t*)realloc(payload, payloadlen);
      memset(payload+offset,0,payloadlen-offset);
      payload[offset++] = usernamelen >> 8;
      payload[offset++] = usernamelen & (0xFF);
      memcpy(payload+offset, broker->username, usernamelen);
    }

  //set password
  if(passwordlen)
    {
      payloadlen += passwordlen+2;
      offset += usernamelen;
      flags |= MQTT_CONNECT_PASSWORD;
      payload = (uint8_t*)realloc(payload, payloadlen);
      memset(payload+offset,0,payloadlen-offset);
      payload[offset++] = passwordlen >> 8;
      payload[offset++] = passwordlen & (0xFF);
      memcpy(payload+offset, broker->password, passwordlen);
    }

  /* variable header */
  //protocol name
  var_header[0] = (0x00);          //string length MSB
  var_header[1] = (0x06);          //string length LSB
  var_header[2] = MQTT_CONNECT_M;  //encoded character data
  var_header[3] = MQTT_CONNECT_Q;
  var_header[4] = MQTT_CONNECT_I;
  var_header[5] = MQTT_CONNECT_s;
  var_header[6] = MQTT_CONNECT_d;
  var_header[7] = MQTT_CONNECT_p;
  //protocol version
  var_header[8] = MQTT_PROTOCOL_VERSION;
  //flags
  var_header[9] = flags;
  //keep-alive
  var_header[10] = broker->keep_alive >> 8;
  var_header[11] = broker->keep_alive & (0xFF);

  /* fixed header */
  uint8_t fix_header[] = {MQTT_TYPE_CONNECT|MQTT_NO_FLAGS, (uint8_t)(sizeof(var_header)+payloadlen)};

  /* full packet */
  packetlen = sizeof(fix_header)+sizeof(var_header)+payloadlen;
  memset(mqtt_send_buffer, 0, MAX_PACKET_LENGTH);
  memcpy(mqtt_send_buffer, fix_header, sizeof(fix_header));
  memcpy(mqtt_send_buffer+sizeof(fix_header), var_header, sizeof(var_header));
  memcpy(mqtt_send_buffer+sizeof(fix_header)+sizeof(var_header), payload, payloadlen);

  /* free memory */
  free(payload);
  payload = NULL;

  /* send packet and return */
  return mqtt_send_packet(broker, (size_t)packetlen, MQTT_TYPE_CONNECT);
}

int mqtt_send_disconnect(broker_handle_t *broker)
{
  uint8_t packet[] = {MQTT_TYPE_DISCONNECT, (0x00)};
  memset(mqtt_send_buffer, 0, MAX_PACKET_LENGTH);
  memcpy(mqtt_send_buffer, packet, sizeof(packet));
  return mqtt_send_packet(broker, sizeof(packet), MQTT_TYPE_DISCONNECT);
}

int mqtt_send_ping(broker_handle_t *broker)
{
  uint8_t packet[] = {MQTT_TYPE_PINGREQ, (0x00)};
  memset(mqtt_send_buffer, 0, MAX_PACKET_LENGTH);
  memcpy(mqtt_send_buffer, packet, sizeof(packet));
  return mqtt_send_packet(broker, sizeof(packet), MQTT_TYPE_PINGREQ);
}

int mqtt_send_packet(broker_handle_t *broker, size_t send_buffer_size, uint8_t type_id)
{
  size_t data_sent;

  if(broker->debug_session)
    printf("send %s\n", mqtt_get_msgtype(type_id));

  data_sent = send(broker->sockfd, mqtt_send_buffer, send_buffer_size, 0);

  if(data_sent != (size_t)-1)
    {
      if(data_sent != send_buffer_size) {
      fprintf(stderr, "mqtt_send_packet(): not send the entire packet\n");
	return -1;
      }
      else {
      if(broker->debug_session) printf("OK\n");
	return SUCCESS;
      }
    }
  else
    {
      printf("FAIL\n");
      fprintf(stderr, "mqtt_send_packet(): %s\n", strerror(errno));
      return -1;
    }

  last_send = time(NULL);
}

int mqtt_send_subscribe(broker_handle_t *broker, const char *topic_name, uint8_t topic_qos)
{
  uint16_t topiclen = strlen(topic_name), packetlen;
  uint8_t offset=0, var_header[2], payload[topiclen+3];
  mqtt_msg_id++;

  //var-header
  var_header[0] = mqtt_msg_id>>8;
  var_header[1] = mqtt_msg_id&(0xFF);

  //payload
  memset(payload,0,sizeof(payload));
  payload[offset++] = topiclen>>8;
  payload[offset++] = topiclen&(0xFF);
  memcpy(payload+offset,topic_name,topiclen);
  offset += topiclen;
  payload[offset] = topic_qos;

  //fixed-header
  uint8_t fix_header[] = {MQTT_TYPE_SUBSCRIBE | MQTT_FLAG_QoS1, (uint8_t)(sizeof(var_header)+sizeof(payload))};

  //packet
  packetlen = sizeof(fix_header)+sizeof(var_header)+sizeof(payload);
  memset(mqtt_send_buffer, 0, MAX_PACKET_LENGTH);
  memcpy(mqtt_send_buffer, fix_header, sizeof(fix_header));
  memcpy(mqtt_send_buffer+sizeof(fix_header), var_header, sizeof(var_header));
  memcpy(mqtt_send_buffer+sizeof(fix_header)+sizeof(var_header), payload, sizeof(payload));

  return mqtt_send_packet(broker, (size_t)packetlen, MQTT_TYPE_SUBSCRIBE);
}

int mqtt_send_publish(broker_handle_t *broker, const char *topic_name, const char *msg, uint8_t qos, uint8_t dup, uint8_t retain)
{
  uint16_t topiclen = strlen(topic_name);
  uint16_t msglen = strlen(msg);
  uint16_t varheaderlen = topiclen+2;
  uint16_t remaininglen, packetlen;
  uint8_t flags = (0x00), offset = 0, *var_header, *fix_header, fh_size = 2;

  //var-header
  var_header = (uint8_t*)malloc(varheaderlen*sizeof(uint8_t));
  memset(var_header,0,varheaderlen);
  var_header[offset++] = topiclen>>8;
  var_header[offset++] = topiclen&(0xFF);
  memcpy(var_header+offset,topic_name,topiclen);

  if(qos == 1 || qos == 2)
    {
      mqtt_msg_id++;
      if(qos == 1)
	flags |= MQTT_FLAG_QoS1;
      else if(qos == 2)
	flags |= MQTT_FLAG_QoS2;
      offset += topiclen;
      varheaderlen += 2;
      var_header = (uint8_t*)realloc(var_header, varheaderlen);
      var_header[offset++] = mqtt_msg_id>>8;
      var_header[offset++] = mqtt_msg_id&(0xFF);
    }

  if(dup)
    flags |= MQTT_FLAG_DUP;

  if(retain)
    flags |= MQTT_FLAG_RETAIN;


  //fixed-header
  remaininglen = varheaderlen + msglen;
  if(remaininglen > 127) fh_size++;
  fix_header = (uint8_t*)malloc(fh_size*sizeof(uint8_t));

  fix_header[0] = MQTT_TYPE_PUBLISH | flags;
  if(remaininglen <= 127)
    fix_header[1] = remaininglen;
  else {
    fix_header[1] = (remaininglen % 128) | (0x80);
    fix_header[2] = remaininglen / 128;
  }

  //packet
  packetlen = fh_size+varheaderlen+msglen;
  memset(mqtt_send_buffer, 0, MAX_PACKET_LENGTH);
  memcpy(mqtt_send_buffer, fix_header, fh_size);
  memcpy(mqtt_send_buffer+fh_size, var_header, varheaderlen);
  memcpy(mqtt_send_buffer+fh_size+varheaderlen, msg, msglen);

  free(fix_header);
  fix_header = NULL;
  free(var_header);
  var_header = NULL;

  return mqtt_send_packet(broker, (size_t)packetlen, MQTT_TYPE_PUBLISH);
}

int mqtt_send_unsubscribe(broker_handle_t *broker, const char *topic_name)
{
  uint16_t topiclen = strlen(topic_name), packetlen;
  uint8_t offset=0, var_header[2], payload[topiclen+2];;
  mqtt_msg_id++;

  //variable-header
  var_header[0] = mqtt_msg_id>>8;
  var_header[1] = mqtt_msg_id&(0xFF);

  //payload
  memset(payload, 0, sizeof(payload));
  payload[offset++] = topiclen>>8;
  payload[offset++] = topiclen&(0xFF);
  memcpy(payload+offset, topic_name, topiclen);

  //fixed-header
  uint8_t fix_header[] = {MQTT_TYPE_UNSUBSCRIBE | MQTT_FLAG_QoS1, (uint8_t)(sizeof(var_header)+sizeof(payload))};

  //packet
  packetlen = sizeof(fix_header)+sizeof(var_header)+sizeof(payload);
  memset(mqtt_send_buffer, 0, MAX_PACKET_LENGTH);
  memcpy(mqtt_send_buffer, fix_header, sizeof(fix_header));
  memcpy(mqtt_send_buffer+sizeof(fix_header), var_header, sizeof(var_header));
  memcpy(mqtt_send_buffer+sizeof(fix_header)+sizeof(var_header), payload, sizeof(payload));

  return mqtt_send_packet(broker, (size_t)packetlen, MQTT_TYPE_UNSUBSCRIBE);
}

int mqtt_send_pubrel(broker_handle_t *broker)
{
  uint8_t var_header[2];
  uint16_t packetlen;
  mqtt_msg_id++;

  //variable-header
  var_header[0] = mqtt_msg_id>>8;
  var_header[1] = mqtt_msg_id&(0xFF);

  //fixed-header
  uint8_t fix_header[] = {MQTT_TYPE_PUBREL | MQTT_FLAG_QoS1, (uint8_t)(sizeof(var_header))};

  //packet
  packetlen = sizeof(fix_header)+sizeof(var_header);
  memset(mqtt_send_buffer, 0, MAX_PACKET_LENGTH);
  memcpy(mqtt_send_buffer, fix_header, sizeof(fix_header));
  memcpy(mqtt_send_buffer+sizeof(fix_header), var_header, sizeof(var_header));

  return mqtt_send_packet(broker, (size_t)packetlen, MQTT_TYPE_PUBREL);
}
