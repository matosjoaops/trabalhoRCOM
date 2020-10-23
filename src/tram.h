#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#pragma once

// Flag
#define FLAG 0x7e

// Control Field
#define INFO_CTRL 0x00
#define SET 0x03
#define DISC 0x0b
#define UA 0x07
#define RR 0x05
#define REJ 0x01
#define R_MASK 0x80
#define S_MASK 0x40

// Address Field
#define COMM_SEND_REP_REC 0x03
#define COMM_REC_REP_SEND 0x01

//Escape sequences
#define ESC_BYTE_1 0x7d
#define ESC_BYTE_2 0x5e
#define ESC_BYTE_3 0x5d

//Sizes
#define NON_INFO_TRAM_SIZE 5

//Process SU tram results
#define DO_NOTHING 0
#define SEND_NEW_DATA 1
#define RESEND_DATA 2

//Last sequential number received/sent
int last_seq;

//Data received
unsigned char ** packet;

//Parse results
struct parse_results
{
    unsigned char received_data[255]; //NULL if it's not info tram
    int tram_size; //size of the tram in bytes
    int duplicate; //boolean to indicate if the tram received is a duplicate or not
    int data_integrity; //boolean to indicate if bcc2 checks out
    int control_bit; //value of the bit in the control_field, either 0 or 1
    int header_validity; //boolean to indicate if the header is valid
};

int r, s;

int last_s,last_r;

long int data_trams_received;

int sender; // boolean that indicates whether the program running is the sender or the receiver 

unsigned char * last_tram_sent;

int last_tram_sent_size;

int last_packet_index;

int packet_size, packet_num;

void setup_initial_values();

unsigned char *generate_info_tram(unsigned char *data, unsigned char address, int array_size);

unsigned char *generate_su_tram(unsigned char address, unsigned char control);

struct parse_results * parse_info_tram(unsigned char *tram, int tram_size);

void process_info_tram_received(struct parse_results * results, int port);

unsigned char * translate_array(unsigned char * array, int offset, int array_size, int starting_point);

void byte_stuff(unsigned char * tram, int * tram_size);

void byte_unstuff(unsigned char * tram, int * tram_size);

int parse_and_process_su_tram(unsigned char * tram, int fd);