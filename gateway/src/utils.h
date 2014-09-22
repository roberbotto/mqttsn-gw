#ifndef __UTILS__
#define __UTILS__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>

#define MAX 100
#define MAX_FRAME 500
#define LOCAL 0
#define EXTERNAL 1
#define BAUD_RATE B38400
#define IDLE 0
#define START1 1
#define START2 2
#define START3 3
#define DATA 4
#define END 5

extern int end, count, longitude, readserial;
extern char atmymy[80], c, str[MAX];
extern const char *serialPort;
extern int thefd;

typedef struct {
  char *buf;
  int size;
}packet_received_t;

typedef struct {
  char *mac;
  char *mqttsn;
}packet_splited_t;

int xbee_init(void);
void xbee_getPacket(packet_received_t *p802);
void xbee_parsePacket(packet_splited_t *pSplited, packet_received_t *p802);
void send_byMac(const char *sensor_mac, const char *msg);

#endif
