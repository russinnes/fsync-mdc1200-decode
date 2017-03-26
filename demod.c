#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "fsync_decode.h" /* and link with mdc_decode.c */ 
#include "mdc_decode.h"

static int integer_only = true;

void fsyncCallBack(int cmd, int subcmd, int from_fleet, int from_unit, int to_fleet, int to_unit, int allflag, \
                   unsigned char *payload, int payload_len, unsigned char *raw_msg, int raw_msg_len, \
                   void *context, int is_fsync2, int is_2400)
    {
        fprintf(stderr,"FLEETSYNC: ");
        fprintf(stderr,"%d ", cmd);
        fprintf(stderr,"%d ", subcmd);
        fprintf(stderr,"%d ", from_fleet);
        fprintf(stderr,"%d ", from_unit);
        fprintf(stderr,"%d ", to_fleet);
        fprintf(stderr,"%d ", to_unit);
        fprintf(stderr,"%d ", allflag);
        fprintf(stderr,"|%.*s| ", payload_len, payload);
        fprintf(stderr,"%d ", payload_len);
        fprintf(stderr,"%.*s ", raw_msg_len, raw_msg);
        fprintf(stderr,"%d ", raw_msg_len);
        fprintf(stderr,"%d ", is_fsync2);
        fprintf(stderr,"%d ", is_2400);
        fprintf(stderr,"\n");
    }
    
static void input_sound(unsigned int sample_rate, unsigned int overlap, int decode_type)
{
    // FSYNC STUFF
    fprintf(stderr, "\nInput Type: System Audio Input\n");
    fsync_decoder_t *f_decoder; 
    int result; 
    f_decoder = fsync_decoder_new(sample_rate); 
    fsync_decoder_set_callback(f_decoder, fsyncCallBack, 0);
    // MDC1200 STUFF
    mdc_decoder_t *m_decoder; 
    unsigned char op, arg, extra0, extra1, extra2, extra3; 
    unsigned short unitID;
    m_decoder = mdc_decoder_new(sample_rate);  
    //
    short buffer[8192];
    float fbuf[16384];
    unsigned int fbuf_cnt = 0;
    int i;
    int error;
    short *sp;
    pa_simple *s;
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16NE;
    ss.channels = 1;
    ss.rate = sample_rate;
    /* Create the recording stream */
    if (!(s = pa_simple_new(NULL, "Fleetsync/MDC1200 Decoder", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        exit(4);
    }
    switch(decode_type){
      case 0: // Fleetsync
        {     
        fprintf(stderr, "Decoder: Fleetsync\n");
        for (;;) {
            i = pa_simple_read(s, sp = buffer, sizeof(buffer), &error);
            if (i < 0 && errno != EAGAIN) {
                perror("read");
                fprintf(stderr, "error 1\n");
                exit(4);
            }
            i=sizeof(buffer);
            if (!i)
                break;
            if (i > 0) {
                if(integer_only)
            {
                    fbuf_cnt = i/sizeof(buffer[0]);
            }
                else
                {
                    for (; (unsigned int) i >= sizeof(buffer[0]); i -= sizeof(buffer[0]), sp++)
                        fbuf[fbuf_cnt++] = (*sp) * (1.0/32768.0);
                    if (i)
                        fprintf(stderr, "warning: noninteger number of samples read\n");
                }
                if (fbuf_cnt > overlap) 
                    {
                    result = fsync_decoder_process_samples(f_decoder, buffer, sizeof(buffer));
                    memmove(fbuf, fbuf+fbuf_cnt-overlap, overlap*sizeof(fbuf[0]));
                    fbuf_cnt = overlap;
                    switch(result) 
                        { 
                        case 0: break; 
                        case -1: exit(1); 
                        } 
                    }
                }
            }
        }
      case 1: // MDC1200
        { 
        fprintf(stderr, "Decoder: MDC1200\n");    
        for (;;) {
            i = pa_simple_read(s, sp = buffer, sizeof(buffer), &error);
            if (i < 0 && errno != EAGAIN) {
                perror("read");
                fprintf(stderr, "error 1\n");
                exit(4);
            }
            i=sizeof(buffer);
            if (!i)
                break;
            if (i > 0) {
                if(integer_only)
            {
                    fbuf_cnt = i/sizeof(buffer[0]);
            }
                else
                {
                    for (; (unsigned int) i >= sizeof(buffer[0]); i -= sizeof(buffer[0]), sp++)
                        fbuf[fbuf_cnt++] = (*sp) * (1.0/32768.0);
                    if (i)
                        fprintf(stderr, "warning: noninteger number of samples read\n");
                }
                if (fbuf_cnt > overlap) 
                    {
                    result = mdc_decoder_process_samples(m_decoder, buffer, sizeof(buffer));
                    memmove(fbuf, fbuf+fbuf_cnt-overlap, overlap*sizeof(fbuf[0]));
                    fbuf_cnt = overlap;
                    switch(result) 
                        { 
                        case 0: break; 
                        case -1: exit(1); 
                        case 1: 
                            mdc_decoder_get_packet(m_decoder, &op, &arg, &unitID); 
                            fprintf(stderr, "MDC1200: %02x %02x %04x\n", op, arg, unitID); 
                            break; 
                        case 2: 
                            mdc_decoder_get_double_packet(m_decoder, &op, &arg, &unitID, &extra0, &extra1, &extra2, &extra3); 
                            fprintf(stderr, "MDC1200: %02x %02x %04x %02x %02x %02x %02x\n", op, arg, unitID, extra0, extra1, extra2, extra3); 
                            break; 
                        }
                    }
                }
            }
        }
      }
    pa_simple_free(s);
}

static void input_file(unsigned int sample_rate, unsigned int overlap, int decode_type)
{
    // FSYNC STUFF
    fprintf(stderr, "\nInput Type: STDIN Raw Samples\n");
    fsync_decoder_t *f_decoder; 
    int result; 
    f_decoder = fsync_decoder_new(sample_rate); 
    fsync_decoder_set_callback(f_decoder, fsyncCallBack, 0);
    // MDC1200 STUFF
    mdc_decoder_t *m_decoder; 
    unsigned char op, arg, extra0, extra1, extra2, extra3; 
    unsigned short unitID;
    m_decoder = mdc_decoder_new(sample_rate);  
    //
    int fd = 0;//STDIN
    int i;
    short buffer[8192];
    float fbuf[16384];
    unsigned int fbuf_cnt = 0;
    short *sp;
    switch(decode_type) {
      case 0: // Fleetsync
        { 
        fprintf(stderr, "Decoder: Fleetsync\n");
        for (;;) {
            i = read(fd, sp = buffer, sizeof(buffer));
            if (i < 0 && errno != EAGAIN) {
                perror("read");
                exit(4);
            }
            if (!i)
                break;
            if (i > 0) {
                if(integer_only)
            {
                    fbuf_cnt = i/sizeof(buffer[0]);
            }
                else
                {
                    for (; (unsigned int) i >= sizeof(buffer[0]); i -= sizeof(buffer[0]), sp++)
                        fbuf[fbuf_cnt++] = (*sp) * (1.0f/32768.0f);
                    if (i)
                        fprintf(stderr, "warning: noninteger number of samples read\n");
                }
                if (fbuf_cnt > overlap) 
                    {
                    result = fsync_decoder_process_samples(f_decoder, buffer, sizeof(buffer));
                    memmove(fbuf, fbuf+fbuf_cnt-overlap, overlap*sizeof(fbuf[0]));
                    fbuf_cnt = overlap;
                    switch(result) 
                        { 
                        case 0: break; 
                        case -1: exit(1); 
                        } 
                    }
                }
            }
        }
      case 1: // MDC1200
        { 
        fprintf(stderr, "Decoder: MDC1200\n");
        for (;;) {
            i = read(fd, sp = buffer, sizeof(buffer));
            if (i < 0 && errno != EAGAIN) {
                perror("read");
                exit(4);
            }
            if (!i)
                break;
            if (i > 0) {
                if(integer_only)
            {
                    fbuf_cnt = i/sizeof(buffer[0]);
            }
                else
                {
                    for (; (unsigned int) i >= sizeof(buffer[0]); i -= sizeof(buffer[0]), sp++)
                        fbuf[fbuf_cnt++] = (*sp) * (1.0f/32768.0f);
                    if (i)
                        fprintf(stderr, "warning: noninteger number of samples read\n");
                }
                if (fbuf_cnt > overlap) 
                    {
                    result = mdc_decoder_process_samples(m_decoder, buffer, sizeof(buffer));
                    memmove(fbuf, fbuf+fbuf_cnt-overlap, overlap*sizeof(fbuf[0]));
                    fbuf_cnt = overlap;
                    switch(result) 
                        { 
                        case 0: break; 
                        case -1: exit(1); 
                        case 1: 
                            mdc_decoder_get_packet(m_decoder, &op, &arg, &unitID); 
                            fprintf(stderr, "MDC1200: %02x %02x %04x\n", op, arg, unitID); 
                            break; 
                        case 2: 
                            mdc_decoder_get_double_packet(m_decoder, &op, &arg, &unitID, &extra0, &extra1, &extra2, &extra3); 
                            fprintf(stderr, "MDC1200: %02x %02x %04x %02x %02x %02x %02x\n", op, arg, unitID, extra0, extra1, extra2, extra3); 
                            break; 
                        }  
                    }
                }
            }
        }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    int sample_rate = 22050;
    int stdin_sample_rate = 12000;
    unsigned int overlap = 0;
    if (argc < 2) // No flags
        {
        fprintf(stderr, "\n\n            Fleetsync / MDC1200 Decoder for Linux              \n");
        fprintf(stderr, "         Run with '-' flag option to use STDIN for input           \n");
        fprintf(stderr, "        STDIN input must be RAW, MONO 16 bit signed integer        \n");
        fprintf(stderr, "    Default (No argument) is System Audio Input (Pulse Audio)     \n\n");
        fprintf(stderr, "      ** NOTE ** - Proper input volume is necessary for decoding   \n");
        fprintf(stderr, "------------------------------------------------------------------------\n");
        fprintf(stderr, "                   Usage: ./fsync_mdc $ (-)          \n");
        fprintf(stderr, "                   $ = 0: Decode Fleetsync           \n");
        fprintf(stderr, "                   $ = 1: Decode MDC1200             \n");
        fprintf(stderr, "                   '-' indicates raw input from STDIN\n\n\n");
        exit(-1);
        }
    int arg = strtol(argv[1], NULL, 10); //0 or 1, Fsync or MDC
    if (argc < 3) // only int for decoder selection passed
        {
            input_sound(sample_rate, overlap, arg);
        }
    if (argc < 4) // only int for decoder selection passed
        {
        if (strcmp("-", argv[2]) == 0)
          {
            input_file(stdin_sample_rate, overlap, arg);
          }
        }


    else
    {
    fprintf(stderr, "\n\n              Fleetsync / MDC1200 Decoder for Linux              \n");
    fprintf(stderr, "         Run with '-' flag option to use STDIN for input           \n");
    fprintf(stderr, "        STDIN input must be RAW, MONO 16 bit signed integer        \n");
    fprintf(stderr, "    Default (No argument) is System Audio Input (Pulse Audio)     \n\n");
    fprintf(stderr, "      ** NOTE ** - Proper input volume is necessary for decoding   \n");
    fprintf(stderr, "------------------------------------------------------------------------\n");
    fprintf(stderr, "                   Usage: ./demod $ (-)          \n");
    fprintf(stderr, "                   $ = 0: Decode Fleetsync           \n");
    fprintf(stderr, "                   $ = 1: Decode MDC1200             \n");
    fprintf(stderr, "                   '-' indicates raw input from STDIN\n\n\n");
    exit(-1);
    }
}
    