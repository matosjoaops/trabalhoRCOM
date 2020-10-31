/* Non-Canonical Input Processing */

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
#include "tram.h"
#include "state_machine.h"
#include "app_layer.h"
#include "link_layer.h"

#define BAUDRATE B38400

void sigalrm_handler(int signo)
{
  if (signo != SIGALRM) fprintf(stderr, "This signal handler shouldn't have been called. signo: %d\n", signo);
  reached_timeout = 1;
}

void set_sigaction()
{
  struct sigaction action;
  action.sa_handler = sigalrm_handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;

  if (sigaction(SIGALRM, &action, NULL) < 0) fprintf(stderr, "Couldn't install signal handler for SIGALRM.\n");
}

int main(int argc, char **argv)
{
  struct stat file_data;
  if (stat(argv[2],&file_data) < 0)
  {
    fprintf(stderr, "Couldn't get file data!\n");
  }

  file_size = file_data.st_size;

  char *file_name = "pinguim_clone.txt";
  packet_size = MAX_PACKET_SIZE;
  int fd = 0;
  timeout = 1;

  //Initialize packet
  packet = (unsigned char **) calloc(MAX_PACKET_ELEMS, sizeof(unsigned char *));

  /*
  for (int i = 0; i < MAX_PACKET_ELEMS; i++)
  {
      packet[i] = (unsigned char *) calloc(MAX_ARRAY_SIZE, sizeof(unsigned char));
  }
  */

  ll = NULL;

  if ((argc < 2) ||
      ((strcmp("/dev/ttyS10", argv[1]) != 0) &&
       (strcmp("/dev/ttyS11", argv[1]) != 0)))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS11\n");
    exit(1);
  }

  unsigned char *fileData = readFile((unsigned char *)argv[2]);

  processFile(fileData);
  free(fileData);
  fd = llopen(10, TRANSMITTER);

  int *t_values = calloc(2, sizeof(int));
  t_values[0] = FILE_SIZE;
  t_values[1] = FILE_NAME;
  int *l_values = calloc(2, sizeof(int));
  l_values[0] = sizeof(file_size);
  l_values[1] = strlen(file_name);
  unsigned char **values = (unsigned char **)calloc(2, sizeof(unsigned char *));

  values[0] = (unsigned char *) &file_size;
  values[1] = (unsigned char *) file_name;
  unsigned char *control_packet = generate_control_packet(START, 2, t_values, l_values, values);
  long control_packet_size = 1 + l_values[0] + l_values[1] + 4;
  free(t_values);
  free(l_values);
  free(values);
  
  // First Control Packet
  set_sigaction();

  llwrite(fd, (char *)control_packet, control_packet_size);

  printf("packet_num = %d", packet_num);

  // File Packets
  for (int i = 0; i < packet_num; i++)
  {
    unsigned char *data_packet = generate_data_packet(i, packet_size, packet[i]);
    llwrite(fd, (char *)data_packet, packet_size + 4);
    free(data_packet);
  }

  // Last Control Packet
  control_packet[0] = END;
  llwrite(fd, (char *)control_packet, control_packet_size);
  free(control_packet);
  llclose(fd);

  for (int i = 0; i < MAX_PACKET_ELEMS; i++)
  {
      free(packet[i]);
  }
  free(packet);
  
  return 0;
}
