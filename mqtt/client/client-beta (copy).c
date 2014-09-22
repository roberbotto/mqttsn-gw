/* Cliente MQTT
 * 
 * Opciones necesarias:
 * 		-h <host> -p <port> -q qos> -r -n -m <mesg> -t <topic> -i <client_id> -u <user> -p <pass> -k <keepalive> -c <clean_session>
 * 
 * Opciones para conectarse:
 * 		-h <host> -p <port> -i <client_id> -u <user> -p <pass> -k <keepalive> -c <clean_session>
 * */

#include "mqtt.h"

#define TRUE 1
#define FALSE 0

const char *host = "localhost";
const char *port = "1883";

void parse_options(broker_handle_t*,int, char**);

int main(int argc, char* argv[]){
  broker_handle_t *broker = malloc(sizeof(broker_handle_t));
  mqtt_init(broker, host, port);
  parse_options(broker,argc, argv);
  
  mqtt_send_connect(broker);
  mqtt_recv_connack(broker);
  
  
  mqtt_send_subscribe(broker,"a/b", 1);
  mqtt_recv_suback(broker);
  
  mqtt_send_unsubscribe(broker, "a/b");
  mqtt_recv_unsuback(broker);
  
  mqtt_send_disconnect(broker);

  close(broker->sockfd);
  free(broker);

  return 0;
}

void parse_options(broker_handle_t *broker, int argc, char** argv){
  int rc;
  while((rc = getopt(argc, argv, "rdh:p:q:i:u:P:k:c?")) != -1){
    switch(rc){
      case 'r':
	broker->will_retain = TRUE;
	break;
      case 'd':
	broker->debug_session = TRUE;
	break;
      case 'h':
	host = optarg;
	break;
      case 'p':
	port = optarg;
	break;
      case 'q':
	broker->will_qos = atoi(optarg);
	break;
      case 'i':
	strcpy(broker->client_id, optarg);
	break;
      case 'u':
	strcpy(broker->username, optarg);
	break;
      case 'P':
	strcpy(broker->password, optarg);
	break;
      case 'k':
	broker->keep_alive = atoi(optarg);
	break;
      case 'c':
	broker->clean_session = FALSE;
	break;
      case '?':
      default:
	fprintf(stderr, "parse_options(): Error during parsing options\n");
	break;
      }
   }
}
