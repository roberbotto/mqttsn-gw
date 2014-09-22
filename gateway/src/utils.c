#include "utils.h"

int end = 0, count = 0, longitude = 0, readserial = 0;
char atmymy[80] = "", c, str[MAX] = "";
const char *serialPort = "/dev/ttyS0";
int thefd;

int xbee_init()
{
  struct termios options;

  /* opening port:
   * O_RDWR - read and write access mode
   * O_ASYNC - generate a signal when in input or
               output becomes possible on this file descriptor
   * O_NOCTTY - for pathnames which refers to a terminal device
   * O_NDELAY - open the file in nonblocking mode
  */
  thefd = open(serialPort, O_RDWR | O_NOCTTY | O_NDELAY | O_ASYNC);
  if (thefd == -1) {
    // could not open port
    fprintf(stderr,"open_port: Unable to open %s\n", serialPort);
  }
  else {
    fcntl(thefd, F_SETFL, O_NDELAY);
    tcgetattr(thefd, &options);
    // set BAUD RATE
    cfsetispeed(&options, BAUD_RATE);
    cfsetospeed(&options, BAUD_RATE);
    // enable the receiver and set local mode...
    options.c_cflag |= (CLOCAL | CREAD);

    // no parity (8 in 1)
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    /* set input mode (non-canonical, no echo,...) */
    options.c_lflag = 0;

    options.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    options.c_cc[VMIN]     = 1;   /* blocking read until 1 char received */

    tcflush(thefd, TCIFLUSH);

    // apply all changes immediately
    tcsetattr(thefd, TCSANOW, &options);
  }

  return thefd;
}

void xbee_getPacket(packet_received_t *p802)
{
  fd_set read_flags;
  char inbuff[MAX_FRAME];      // Buffer to read incoming data into
  char *offset;
  int err, sizepacket=0, finish=0;
  unsigned int state = IDLE;

  struct timeval waitd;

  offset = &inbuff[0];

  //buffer
  while(1)
    {
      waitd.tv_sec = 1;     // Make select wait up to 1 second for data
      waitd.tv_usec = 0;    // and 0 milliseconds.
      FD_ZERO(&read_flags); // Zero the flags ready for using
      FD_SET(thefd, &read_flags);

      err = select(thefd+1, &read_flags, (fd_set*)0, (fd_set*)0, &waitd);

      if(err < 0) continue;
      if(FD_ISSET(thefd, &read_flags)) {
	FD_CLR(thefd, &read_flags);
	sizepacket = read(thefd, offset, 1);
	if (sizepacket <= 0) {
	  close(thefd);
	  break;
	}
	else {
	  switch(state)
	    {
	    case IDLE:
	      if (*offset==(0x23)){
		state=START1;
		offset++;
	      } else state = IDLE;
	      break;
	    case START1:
	      if (*offset==(0x2d)){
		state=START2;
		offset++;
	      } else state = IDLE;
	      break;
	    case START2:
	      if (*offset==(0x4d)){
		state=START3;
		offset++;
	      } else state = IDLE;
	      break;
	    case START3:
	      if (*offset==(0x41)){
		state=DATA;
		offset++;
	      } else state = IDLE;
	      break;
	    case DATA:
	      if (*offset==(0x0a)){
		state=END;
		//fragment received;
	      }else {
		offset++;
		break;
	      }
	    case END:
	      p802->size = (offset-&inbuff[0])+1;
	      p802->buf = (char*)malloc(p802->size*sizeof(char));
	      memcpy(p802->buf, inbuff, p802->size);

	      finish = 1;
	      offset = &inbuff[0];

	    default:
	      state = IDLE;

	    }
	}
      }
      if(finish) break;
    }
}

void send_byMac(const char* sensor_mac, const char* msg)
{
  char mymy[15] = "";
  char macHigh[15] = "";
  char macLow[15] = "";

  fprintf(stderr, "Preparing the module... \n");
  // divide mac that comes from argv[] in:
  // 32 most significant bits and
  // 32 less significant bits
  strncpy(macHigh, sensor_mac, 8);
  strncpy(macLow, sensor_mac+8, 8);

  // at mode
  write(thefd,"+++",3);
  sleep(2);

  /*
   * An at command is composed by:
   * "at"+"parameter to configure"+"value"+"\r"+"\n" -> to set value
   * "at"+"parameter to configure"+"\r"+"\n" -> to get value
   */

  // make atdh & atdl command
  // atdh -> set 32 most significant bits
  // atdl -> set 32 less significant bits
  char ath[20] = "atdh";
  strcat (ath, macHigh);
  strcat (ath, "\r\n");
  char atl[20] = "atdl";
  strcat (atl, macLow);
  strcat (atl, "\r\n");

  // set atdh
  write(thefd, ath, 14);
  usleep(70000);

  // set atdl
  write(thefd, atl, 14);
  usleep(70000);

  // save my atmy to be restored after send message
  readserial=1;
  write(thefd,"atmy\r\n",6);
  //usleep(70000);
  usleep(170000);
  strcpy (mymy, atmymy);
  readserial=0;

  // set atmy as ffff (network id)
  // to send in 64 bit mode, the network id has to be set to ffff
  write(thefd,"atmyffff\r\n",10);
  usleep(70000);

  // leave at mode -> close connection
  write(thefd,"atcn\r",5);
  usleep(70000);


  fprintf(stderr, "Sending message... \n");
  // send message -> write throught serial port
  // set the waspmote API headers
  int id = 48;
  int numFragmentos = 1;
  int sharp = 35;
  int idType = 2;
  char ni[] = "meshlium#";
  write(thefd, &id, 1);
  write(thefd, &numFragmentos, 1);
  write(thefd, &sharp, 1);
  write(thefd, &idType, 1);
  write(thefd, ni, 9);
  // send message
  if(strlen(msg) >= 60)
    longitude = 60;
  else
    longitude = strlen(msg);

  write ( thefd, msg, longitude);

  sleep(5);
  fprintf(stderr, "Returning to original values... \n");
  // at mode
  write(thefd,"+++",3);
  sleep(2);

  // make atmy command to be restored
  char atmy[20] = "atmy";
  strcat (atmy, mymy);
  strcat (atmy, "\r\n");

  // restore atmy
  write(thefd,atmy,10);
  usleep(70000);

  // leave at mode -> close connection
  write(thefd,"atcn\r",5);
  usleep(70000);
}


void xbee_parsePacket(packet_splited_t *pSplited, packet_received_t *p802)
{
  char *_macStart, *_mqttStart, *aux;
  char mac[22], mqttsn[255];
  int index, error=0, k;
  char pAux[p802->size-1];

  memcpy(pAux, p802->buf, sizeof(pAux));

  aux = strpbrk(pAux, "-MAC");
  if(aux == NULL)
    error = 1;

  //Get mac value
  memset(mac, 0, sizeof(mac));
  if (!error) {
    _macStart=strchr(aux,':');
    if (_macStart != NULL) {
      index=_macStart-aux;
      memcpy(mac,aux+index+1,22);
      mac[22] = '\0';
    }else
      error = 1;
  }

  index = index+2+22+4;

  //Get mqtt value
  memset(mqttsn, 0, sizeof(mqttsn));
  if (!error) {
    _mqttStart=strchr(aux+index,':');
    size_t mqttsn_size = *(_mqttStart+2);
    if (_mqttStart != NULL) {
      memcpy(mqttsn,aux+index+2, mqttsn_size+1);
    } else
      error = 1;
  }

  if (error) {
    fprintf(stderr, "ERROR - Unable to parse received frame: %s\n", pAux);
    exit(-1);
  } else {
    pSplited->mac = (char*)malloc((sizeof(mac)-6)*sizeof(char));
    pSplited->mqttsn = (char*)malloc(mqttsn[1]*sizeof(char));
    memset(pSplited->mac, 0, (sizeof(mac)-6));
    memset(pSplited->mqttsn, 0, mqttsn[1]);
    int real_index = 0;

    for(k=0; k<(int)sizeof(mac); k++) {
      if (k!=0 && k!=9 && k!=10 && k!=19 && k!=20 && k!=21) {
	pSplited->mac[real_index] = mac[k];
	real_index++;
      }
    }

    for(k=1; k<=mqttsn[1]; k++)
      pSplited->mqttsn[k-1] = mqttsn[k];
  }
}
