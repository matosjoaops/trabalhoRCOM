/* LINK LAYER */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define MAX_TRAM_SIZE 2
#define BAUDRATE B38400

struct termios oldtio, newtio;

typedef struct
{
  char *port;
  int baudRate;
  unsigned int sequenceNumber;
  unsigned int timeout;
  unsigned int numTransmissions;
  char frame[MAX_TRAM_SIZE];
} link_layer;
static link_layer *ll = NULL;
int ll_init(char *port, int baudRate, unsigned int timeout, unsigned int numTransmissions)
{
  if (ll == NULL)
    ll = malloc(sizeof(link_layer));
  else
    printf("Link Layer Already Initialized\n");
  ll->port = port;
  ll->baudRate = baudRate;
  ll->timeout = timeout;
  ll->numTransmissions = numTransmissions;
  ll->sequenceNumber = 0;
  printf("Link Layer Initialized!\n");
  return 0;
}
int ll_open_serial_port(int fd)
{
  fd = open(ll->port, O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(ll->port);
    exit(-1);
  }

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = ll->timeout * 10; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 5;                 /* blocking read until 5 chars received */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) proximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");
  return fd;
}

int llopen(int fd)
{
  /*
  Open serial port device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
  */
  fd = ll_open_serial_port(fd);
  // TODO
  // SEND SET
  // READ UA
  return fd;
}

void ll_close_serial_port(int fd)
{
  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }
  close(fd);
}

void llclose(int fd)
{
  // TODO
  // SEND DISC
  // READ DISC
  // SEND UA
  ll_close_serial_port(fd);
}