#include "tram.h"
#include "state_machine.h"

void setup_initial_values()
{  
    /*
    r = 1;
    s = 0;
    last_s = -1;
    last_r = -1;*/
    last_seq = -1;
    reached_timeout = 0;
    if (!sender) data_trams_received = 0;
}

unsigned char *generate_info_tram(char *data, unsigned char address, int array_size)
{
    unsigned char *tram = calloc((6 + array_size), sizeof(unsigned char));

    tram[0] = FLAG;
    tram[1] = address;

    unsigned char actual_control = INFO_CTRL;
    
    if (last_seq == -1 || last_seq == 1) last_seq = 0;
    else if (last_seq == 0) last_seq = 1;

    if (last_seq == 1)
        actual_control |= S_MASK;
    /*
    if (s > 0)
    {
        actual_control |= S_MASK;
        s--;
    }
    else
        s++;
    */
    tram[2] = actual_control;
    tram[3] = address ^ actual_control;

    unsigned char bcc2 = 0x00;

    for (int i = 4; i < (array_size + 4); i++)
    {
        tram[i] = data[i - 4];
        bcc2 ^= tram[i];
    }

    tram[4 + array_size] = bcc2;
    tram[5 + array_size] = FLAG;

    //Storing the last info tram sent
    last_tram_sent = tram;
    last_tram_sent_size = array_size + 6;

    return tram;
}

unsigned char *generate_su_tram(unsigned char address, unsigned char control, int dup)
{
    unsigned char *tram = calloc(5 , sizeof(unsigned char));
    tram[0] = FLAG;
    tram[1] = address;

    unsigned char actual_control = control;

    if (control == REJ && last_seq == 1)
        actual_control = control |= R_MASK;
    else if (control == RR)
    {
        if (!dup)
        {
            if (last_seq == 1) last_seq--;
            else
            {
                last_seq++;
                actual_control = control |= R_MASK;
            }
        }
        else
        {
            if (last_seq == 1) actual_control = control |= R_MASK;
        }
        
        
    }
    /*
    if (control == RR || control == REJ)
    {
        if (r > 0)
        {
            actual_control |= R_MASK;
            r--;
        }
        else
            r++;
    }
    */
    tram[2] = actual_control;
    tram[3] = address ^ actual_control;
    tram[4] = FLAG;

    return tram;
}

int parse_and_process_su_tram(unsigned char *tram, int fd)
{
    if (tram == NULL) return TIMED_OUT;
 
    unsigned char *response;
    int res;
    int result = DO_NOTHING; 

    switch (tram[1])
    {
    case SET:
    {
        printf("Received request to start communication. Acknowledging.\n");
        response = generate_su_tram(COMM_SEND_REP_REC, UA,0);
        break;
    }
    case UA:
    {
        if (sender)
        {
            printf("Communication start request was acknowledged. Starting to send data.\n");
            return SEND_NEW_DATA;
        }
        else
        {
            printf("Communication ended.\n");
            return DO_NOTHING;
        }
        break;
    }
    case DISC:
    {
        if (sender)
        {
            printf("Communication end request was acknowledged. Acknowledging end.\n");
            response = generate_su_tram(COMM_REC_REP_SEND, UA,0);
        }
        else
        {
            printf("Received request to end communication. Ending communication.\n");
            response = generate_su_tram(COMM_REC_REP_SEND, DISC,0);
        }
        break;
    }
    case REJ:
    {
        printf("Last info packet sent had issues. Resending.\n");
        return RESEND_DATA;
        break;
    }
    case (REJ | R_MASK):
    {
        printf("Last info packet sent had issues. Resending.\n");
        return RESEND_DATA;
        break;
    }
    case RR:
    {
        printf("Last info packet sent had no issues. Processing.\n");
        return SEND_NEW_DATA;
        break;
    }
    case (RR | R_MASK):
    {
        printf("Last info packet sent had no issues. Processing.\n");
        return SEND_NEW_DATA;
        break;
    }
    default:
        printf("Invalid control byte!\n");
    }

    res = write(fd, response, NON_INFO_TRAM_SIZE);
    printf("%d Bytes Written\n", res);
    free(response);
    return result;
}

struct parse_results *parse_info_tram(unsigned char *tram, int tram_size)
{   

    //Tram must be unstuffed before being passed to this function, flags should not be included in the tram passed
    struct parse_results *result = calloc(1, sizeof(struct parse_results));
    result->received_data = calloc(max_array_size, sizeof(unsigned char));
    //Setting default values
    //result->received_data = NULL;
    result->tram_size = tram_size;
    result->duplicate = 0;
    result->data_integrity = 1;
    result->control_bit = 0;
    result->header_validity = 1;

    //unsigned char *data_parsed = calloc(tram_size - 4, sizeof(unsigned char));

    if ((tram[0] != COMM_SEND_REP_REC)                                //Checks if the second byte matches one of the possible values for the address field
        || (tram[1] != INFO_CTRL && tram[1] != (INFO_CTRL | S_MASK))) //Checks if the third byte matches one of the possible values for the control field
        result->header_validity = 0;

    unsigned char bcc1 = tram[0] ^ tram[1];

    if (bcc1 != tram[2])
        result->header_validity = 0;

    switch (tram[1])
    {
    case INFO_CTRL:
    {
        if (last_seq == -1)
            last_seq = 0;
        else if (last_seq == 1)
            result->duplicate = 1;
        memcpy(result->received_data,&tram[3],(tram_size - 4));
        printf("Data Tram Received.\n");
        break;
    }
    case (INFO_CTRL | S_MASK):
    {
        if (last_seq == -1)
            last_seq = 1;
        else if (last_seq == 0)
            result->duplicate = 1;
        memcpy(result->received_data,&tram[3],(tram_size - 4));
        printf("Data Tram Received.\n");
        break;
    }
    default:
        result->header_validity = 0;
    }

    //result->received_data = data_parsed;

    unsigned char bcc2 = 0x00;

    for (int i = 3; i < (tram_size - 1); i++)
    {
        bcc2 ^= tram[i];
    }

    if (bcc2 != tram[tram_size - 1])
        result->data_integrity = 0;
    
    return result;
}

char * process_info_tram_received(struct parse_results *results, int port)
{
    unsigned char *response;
    char * result;
    result = calloc(max_array_size,sizeof(unsigned char));
    int response_size = 0;

    if (!results->header_validity)
        return NULL;
    if (results->header_validity && results->data_integrity)
    {
        
        if (!results->duplicate)
        {
            memcpy(result, results->received_data, results->tram_size - 4);
        }
        else result = NULL;
            
        response = generate_su_tram(COMM_SEND_REP_REC, RR,0);
        response_size = 5;
    }

    if (results->header_validity && !results->data_integrity)
    {
        if (!results->duplicate)
        {
            response = generate_su_tram(COMM_SEND_REP_REC, REJ,0);
            fprintf(stderr, "Data had errors, responding with REJ.\n");
        }
        else
        {
            response = generate_su_tram(COMM_SEND_REP_REC, RR, 1);
            fprintf(stderr, "Data had errors but was duplicate, responding with RR.\n");
        }
        result = NULL; 
        response_size = 5;
    }

    int res = write(port, response, response_size);
    free(response);
    res = res;
    printf("%d Bytes Written\n", res);
    return result;
}

unsigned char *translate_array(unsigned char *array, int offset, int array_size, int starting_point)
{
    unsigned char *new_array = calloc(array_size + offset, sizeof(unsigned char));
    
    for (int i = 0; i < (array_size); i++)
    {
        if (i < starting_point)
        {
            new_array[i] = array[i];
        }
        else if (offset > 0)
        {
            new_array[i + offset] = array[i];
        }
        else
        {
            if (i == (array_size + offset)) break;
            new_array[i] = array[i - offset];
        }
    }
    
    free(array);
    return new_array;
}

unsigned char * byte_stuff(unsigned char *tram, int *tram_size)
{
    for (int i = 4; i < ((*tram_size) - 1); i++)
    {
        if (tram[i] == FLAG)
        {
            tram = translate_array(tram, 1, (*tram_size), i);
            (*tram_size)++;
            tram[i] = ESC_BYTE_1;
            tram[++i] = ESC_BYTE_2;
            
        }
        else if (tram[i] == ESC_BYTE_1)
        {
            tram = translate_array(tram, 1, (*tram_size), i);
            (*tram_size)++;
            tram[i] = ESC_BYTE_1;
            tram[++i] = ESC_BYTE_3;
        }
    }
    
    return tram;
}

unsigned char * byte_unstuff(unsigned char *tram, int *tram_size)
{
    for (int i = 3; i < (*tram_size); i++)
    {
        if (tram[i] == ESC_BYTE_1 && tram[i + 1] == ESC_BYTE_2)
        {
            tram = translate_array(tram, -1, (*tram_size), i);
            (*tram_size)--;
            tram[i] = FLAG;
        }
        else if (tram[i] == ESC_BYTE_1 && tram[i + 1] == ESC_BYTE_3)
        {
            tram = translate_array(tram, -1, (*tram_size), i);
            (*tram_size)--;
            tram[i] = ESC_BYTE_1;
        }
    }
    return tram;
}