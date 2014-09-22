#include "mqtt.h"
#include "utils.h"
#include "mqttsnSF.h"
#include <iostream>
#include <map>

using namespace std;

#define CONNECT 10
#define MAX_CONNECTIONS 50

const char *host = "161.67.106.49";
const char *port = "1883";

int max(int a, int b);
char* string2charP(std::string str);
void updateMaps(std::map<string,int> &macs, std::map<int,string> &socks, std::map<string,broker_handle_t*> &brokers, int sock, char *mac, broker_handle_t *broker);
int exists_mac(std::map<string,int> &macs, char *mac);
int exists_sock(std::map<int,string> &socks, int sock);
std::string getMac_bySocket(std::map<int,string> &socks, int sock);
broker_handle_t* getBroker_byMac(std::map<string,broker_handle_t*> &brokers, char *mac);
void print_maps(std::map<string,int> &macs, std::map<string,broker_handle_t*> &brokers, std::map<int,string> &socks);
void clean_clientInfo(std::map<string,int> &macs, std::map<int,string> &socks, std::map<string,broker_handle_t*> &brokers, char *mac);

int main(int argc, char *argv[])
{
  int xbeefd, retval, maxfd = -1, ncon=0;
  int sockets[MAX_CONNECTIONS];

  std::map<string, int> macsMap;
  std::map<int, string> socksMap;
  std::map<string, broker_handle_t*> brokersMap;

  packet_splited_t *pSplited = (packet_splited_t*)malloc(sizeof(packet_splited_t));
  broker_handle_t *broker = (broker_handle_t*)malloc(sizeof(broker_handle_t));
  packet_received_t *p802 = (packet_received_t*)malloc(sizeof(packet_received_t));

  fd_set rfds;

  /*get the xbee descriptor*/
  xbeefd = xbee_init();
  maxfd = max(xbeefd,maxfd);

  //loop
  for(;;)
    {
      int error = 0;

      //prepare select()
      FD_ZERO(&rfds);
      FD_SET(xbeefd, &rfds);
      for(int i=0; i<ncon; i++)
	FD_SET(sockets[i], &rfds);

      retval = select(maxfd+1, &rfds, (fd_set*)0, (fd_set*)0, NULL);

      if(retval == -1 && errno==EINTR) continue;
      if(retval < 0) {
	perror("select()");
	exit(1);
      }

      //available data on xbee descriptor
      if(FD_ISSET(xbeefd, &rfds))
	{
	  fprintf(stderr, "Available data on xbee descriptor.\n");

	  xbee_getPacket(p802);
	  xbee_parsePacket(pSplited, p802);

	  //analizar el tipo de mensaje que nos ha llegado
	  char msgtype = pSplited->mqttsn[1];
	  uint8_t msg_register = 0;
	  uint8_t will_connect = 0;
	  uint8_t disconnect_with_duration = 0, publish_m1 = 0;
	  uint8_t ping_with_clientid = 0;

	  if(msgtype == MQTTSN_TYPE_CONNECT)
	    {
	      fprintf(stderr, "--Connect msg from Waspmote\n");

	      //check if will features are active
	      will_connect = (pSplited->mqttsn[2])&(MQTTSN_FLAG_WILL);
	      //init mqtt broker connection and store fd into sockets array
	      sockets[ncon] = mqtt_init(broker, host, port);
	      //add this socket to fd select group
	      FD_SET(sockets[ncon], &rfds);
	      maxfd = max(broker->sockfd, maxfd);
	      //update maps
	      updateMaps(macsMap, socksMap, brokersMap, sockets[ncon], pSplited->mac, broker);
	      //init topics
	      initTopics_byDefault(pSplited->mac);
	      ncon++;
	    }

	  else
	    {
	      fprintf(stderr, "--Not connect msg from Waspmote\n");

	      //check if incoming MAC exist in macsMap
	      if(exists_mac(macsMap, pSplited->mac) == 1) {
		//it's a valid msg, check for special msg types
		if(msgtype == MQTTSN_TYPE_REGISTER) {
		  msg_register = 1;
		}
		else if(msgtype == MQTTSN_TYPE_DISCONNECT) {
		  if(pSplited->mqttsn[0] == 4)
		    disconnect_with_duration = 1;
		}
		else if(msgtype == MQTTSN_TYPE_PINGREQ) {
		  if(pSplited->mqttsn[0] > 2)
		    ping_with_clientid = 1;
		}
	      }
	      else {
		//check if the msg is a publish with qos -1, if not -> error
		if(msgtype == MQTTSN_TYPE_PUBLISH) {
		  int qos = (pSplited->mqttsn[2])&(MQTTSN_FLAG_QoSm1);
		  if(qos == MQTTSN_FLAG_QoSm1)
		    publish_m1 = 1;
		}
		else
		  error = 1;
	      }
	    }

	  //NOW WE KNOW IF CLIENT REQUIRES A SERVER OR A FORWARDER

	  if(!error) {
	    //SERVER
	    if(msg_register) {
	      //treat the registration of a topic name
	      fprintf(stderr, "---Register\n");
	      register_handler(pSplited->mqttsn, pSplited->mac);
	    } else if(disconnect_with_duration) {
	      //tratar con los diferentes estados
	    } else if(will_connect) {
	      //llamar a la funcion que realice esta interaccion
	    } else if(ping_with_clientid) {
	      //tratar los diferentes estados
	    } else
	      {
		//FORWARDER
		fprintf(stderr, "---Calling mqttsn2mqtt\n");
		broker_handle_t *broker_aux;

		if(publish_m1) {
		  //init broker and socket for this special case
		  broker_aux = (broker_handle_t*)malloc(sizeof(broker_handle_t));
		  mqtt_init(broker_aux, host, port);
		  //init topics for this special case
		  initTopics_byDefault(pSplited->mac);
		  //call to conversion function
		  error = mqttsn2mqtt(pSplited->mqttsn, broker_aux, msgtype, pSplited->mac);
		  //free the mem of this special case
		  close(broker_aux->sockfd);
		  free(broker_aux);
		  broker_aux=NULL;
		}
		else {
		  broker_aux = getBroker_byMac(brokersMap, pSplited->mac);
		  error = mqttsn2mqtt(pSplited->mqttsn, broker_aux, msgtype, pSplited->mac);
		}

		if(error == -1)
		  fprintf(stderr, "mqttsn2mqtt(): Error while converting: %d\n",error);
		else if(msgtype == MQTTSN_TYPE_DISCONNECT) {
		  //clean all data for this client
		  close(broker_aux->sockfd);
		  clean_clientInfo(macsMap, socksMap, brokersMap, pSplited->mac);
		  ncon--;
		}
	      }
	  }
	}

      for(int j=0; j<ncon; j++) {
	//available data on socket j
	if(FD_ISSET(sockets[j], &rfds)) {

	  printf("\nAvailable data on socket %d\n", sockets[j]);
	  std::string aux;

	  //check if sockets exists on socksMap
	  if(exists_sock(socksMap, sockets[j]))
	    {
	      //get the mac of this connection
	      aux = getMac_bySocket(socksMap, sockets[j]);
	      char *mac = string2charP(aux);

	      //get the broker of this connection
	      broker_handle_t *broker_aux = getBroker_byMac(brokersMap, mac);

	      //receive the incoming packet
	      mqtt_recv_packet(broker_aux, 255, 0x1E);

	      //call to conversion function
	      fprintf(stderr, "---Calling mqtt2mqttsn\n");
	      mqtt2mqttsn(mac);
	    }
	  else {
	    char tmpbuf[128];
	    recv(sockets[j], tmpbuf, sizeof(tmpbuf), 0);
	  }
	}
      }

    }

  free(p802);
  free(pSplited);
  free(broker);
  p802=NULL;
  pSplited=NULL;
  broker=NULL;

  return 0;
}

int max(int a, int b)
{
  return (a>b)?a:b;
}

char* string2charP(std::string str)
{
  char *ret;
  ret = new char[str.size()+1];
  std::copy(str.begin(), str.end(), ret);
  ret[str.size()] = '\0';
  return ret;
}

void updateMaps(std::map<string,int> &macs, std::map<int,string> &socks, std::map<string,broker_handle_t*> &brokers, int sock, char *mac, broker_handle_t *broker)
{
  broker_handle_t *bAux = (broker_handle_t*)malloc(sizeof(broker_handle_t));
  memcpy(bAux, broker, sizeof(broker_handle_t));

  std::string mAux(mac,16);
  //remove any ocurrence of previous sockets with the same mac
  for(std::map<int,string>::const_iterator it = socks.begin(); it != socks.end(); it++) {
    if(it->second == mAux)
      socks.erase(it->first);
  }
  //update
  macs[mAux] = sock;
  socks[sock] = mAux;
  brokers[mAux] = bAux;
}

int exists_mac(std::map<string, int> &macs, char *mac)
{
  std::string aux(mac,16);
  if(macs.count(aux) == 1)
    return 1;
  else
    return 0;
}

int exists_sock(std::map<int,string> &socks, int sock)
{
  if(socks.count(sock) == 1)
    return 1;
  else
    return 0;
}

std::string getMac_bySocket(std::map<int,string> &socks, int sock)
{
  std::string aux;
  std::map<int,string>::const_iterator search = socks.find(sock);
  if(search != socks.end())
    aux = search->second;
  else
    fprintf(stderr, "getMac_bySocket(): Error, socket not found on map.\n");

  return aux;
}

broker_handle_t* getBroker_byMac(std::map<string,broker_handle_t*> &brokers, char *mac)
{
  broker_handle_t *bAux;
  std::string aux(mac,16);
  std::map<string,broker_handle_t*>::const_iterator search = brokers.find(aux);
  if(search != brokers.end()) {
    bAux = search->second;
  }
  else
    fprintf(stderr, "getBroker_byMac(): Error, mac not found on brokersMap.\n");

  return bAux;
}

void clean_clientInfo(std::map<string,int> &macs, std::map<int,string> &socks, std::map<string,broker_handle_t*> &brokers, char *mac)
{
  int sock = -1;
  std::string aux(mac,16);
  std::map<string,int>::const_iterator search = macs.find(aux);

  if(search != macs.end())
    sock = search->second;
  else
    fprintf(stderr, "clean_clientInfo(): Error, mac not found on macsMap.\n");

  if(sock != -1) {
    macs.erase(aux);
    socks.erase(sock);
    std::map<string,broker_handle_t*>::const_iterator search2 = brokers.find(aux);
    if(search2 != brokers.end()) {
      free(search2->second);
    }
    brokers.erase(aux);
  }
}

void print_maps(std::map<string, int> &macs, std::map<string,broker_handle_t*> &brokers, std::map<int,string> &socks)
{
  fprintf(stderr, "macsMap:\n");
  for(std::map<string,int>::const_iterator it = macs.begin(); it!=macs.end(); ++it)
    std::cout << it->first << " " << it->second << std::endl;

  fprintf(stderr, "brokersMap:\n");
  for(std::map<string,broker_handle_t*>::const_iterator it = brokers.begin(); it!=brokers.end(); ++it)
    std::cout << it->first << " " << it->second->sockfd << std::endl;

  fprintf(stderr, "socksMap:\n");
  for(std::map<int,string>::const_iterator it = socks.begin(); it!=socks.end(); ++it)
    std::cout << it->first << " " << it->second << std::endl;
}
