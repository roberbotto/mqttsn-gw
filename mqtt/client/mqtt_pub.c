/* MQTT Publish Client
 *
 * Options:
 *    -h <host> -p <port> -q <qos> -d
 *    -r -n -m <mesg> -t <topic>
 *    -i <client_id> -u <user> -P <pass>
 *    -k <keepalive> -c <clean_session>
 * */

#include "mqtt.h"
#include <signal.h>

#define TRUE 1
#define FALSE 0

const char *host = "localhost";
const char *port = "1883";
const char *topic = NULL;
const char *msg = NULL;
int error = FALSE;
int running = TRUE;
int qos = 0;

void parse_options(broker_handle_t*,int, char**);
void usage(broker_handle_t*);
void termination_handler (int);

int main(int argc, char* argv[]){

  uint8_t *message = NULL;
  broker_handle_t *broker = malloc(sizeof(broker_handle_t));

  // Setup signal handlers
  signal(SIGTERM, termination_handler);
  signal(SIGINT, termination_handler);
  signal(SIGHUP, termination_handler);

  mqtt_init(broker, host, port);
  parse_options(broker, argc, argv);

  if (!error) {
    // connect to broker
    mqtt_send_connect(broker);
    mqtt_recv_connack(broker);
    // publish to a topic
    mqtt_send_publish(broker, "a/b", "Hello World!", qos, 0, 0);
    if(qos == 1)
      mqtt_recv_puback(broker);
    else if(qos == 2) {
      mqtt_recv_pubrec(broker);
      mqtt_send_pubrel(broker);
      mqtt_recv_pubcomp(broker);
    }
    // disconnect
    mqtt_send_disconnect(broker);
  }
  mqtt_destroy(broker);

  return 0;
}

void parse_options(broker_handle_t *broker, int argc, char** argv){
  int rc;
  while((rc = getopt(argc, argv, "rdh:t:m:p:q:i:u:P:k:c?")) != -1){
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
      qos = atoi(optarg);
      break;
    case 't':
      topic = optarg;
      break;
    case 'm':
      msg = optarg;
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
      usage(broker);
      break;
    }
  }
  if(!topic || !msg) usage(broker);
}

void usage(broker_handle_t *broker)
{
  error = 1;
  fprintf(stderr, "Usage: mqtt_pub [opts] -t <topic> -m <message>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, " -c Disable 'clean session' (store subscription and pending messages when client disconnects).\n");
  fprintf(stderr, " -d Enable debug messages.\n");
  fprintf(stderr, " -h <host> MQTT host to connect to. Defaults to '%s'.\n", host);
  fprintf(stderr, " -i <clientid> ID to use for this client.\n");
  fprintf(stderr, " -k <keepalive> keep alive in seconds for this client. Defaults to %d.\n", broker->keep_alive);
  fprintf(stderr, " -p <port> Network port to connect to. Defaults to %s.\n", port);
  fprintf(stderr, " -t <topic> MQTT topic name to publish to.\n");
  fprintf(stderr, " -q <qos> MQTT QoS to use in publish process\n");
  fprintf(stderr, " -u <username> if user access control is enabled in broker,this represent the username of the client.\n");
  fprintf(stderr, " -P <password> password for this username.\n");
  fprintf(stderr, " -m <message> message to publish.\n");
}

/* Function obtained from: https://github.com/njh/mqtt-sn-tools */
void termination_handler (int signum)
{
  switch(signum) {
  case SIGHUP:
    fprintf(stderr, "Got hangup signal.\n");
    break;
  case SIGTERM:
    fprintf(stderr, "Got termination signal.\n");
    break;
  case SIGINT:
    fprintf(stderr, "Got interupt signal.\n");
    break;
  }
  // Signal the main thead to stop
  running = FALSE;
}
