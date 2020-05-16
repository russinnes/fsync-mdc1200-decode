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
#include <time.h>

// Network
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// and link the following with mdc_decode.c on compile!
#include "fsync_decode.h"
#include "mdc_decode.h"

#define UDPPORT 9101



void sendJsonUDP(char *message) {

    int sockfd;
    struct sockaddr_in servaddr;
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        fprintf(stderr, "Socket creation error\n");
        return;
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(UDPPORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;
    int n, len;
    sendto(sockfd, message, strlen(message), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    close(sockfd);
    return;

}




void fsyncCallBack(int cmd, int subcmd, int from_fleet, int from_unit, int to_fleet, \
                    int to_unit, int allflag, unsigned char *payload, int payload_len, \
                    unsigned char *raw_msg, int raw_msg_len, \
                    void *context, int is_fsync2, int is_2400){
    char json_buffer[2048];
    printf("Timestamp: %d\n",(int)time(NULL));
    snprintf(json_buffer, sizeof(json_buffer), "{\"type\":\"FLEETSYNC\","\
                                        "\"timestamp\":\"%d\","\
                                         "\"cmd\":\"%d\","\
                                         "\"subcmd\":\"%d\","\
                                         "\"from_fleet\":\"%d\","\
                                         "\"from_unit\":\"%d\","\
                                         "\"to_fleet\":\"%d\","\
                                         "\"to_unit\":\"%d\","\
                                         "\"all_flag\":\"%d\","\
                                         "\"payload\":\"%.*s\","\
                                         "\"fsync2\":\"%d\","\
                                         "\"2400\":\"%d\"}", (int)time(NULL),cmd,subcmd,from_fleet,from_unit, \
                                                            to_fleet, to_unit, allflag, \
                                                            payload_len, payload, \
                                                            is_fsync2, is_2400) ;
    fprintf(stdout, "%s\n", json_buffer);
    sendJsonUDP(json_buffer);
}

void mdcCallBack(int numFrames, unsigned char op, unsigned char arg, unsigned short unitID,\
                  unsigned char extra0, unsigned char extra1, unsigned char extra2, \
                  unsigned char extra3, void *context){
    char json_buffer[2048];
    snprintf(json_buffer, sizeof(json_buffer), "{\"type\":\"MDC1200\","\
                                        "\"timestamp\":\"%d\","\
                                         "\"op\":\"%02x\","\
                                         "\"arg\":\"%02x\","\
                                         "\"unitID\":\"%04x\","\
                                         "\"ex0\":\"%02x\","\
                                         "\"ex1\":\"%02x\","\
                                         "\"ex2\":\"%02x\","\
                                         "\"ex3\":\"%02x\"}", (int)time(NULL), op, arg, unitID,extra0, \
                                         extra1, extra2, extra3);

    fprintf(stdout, "%s\n", json_buffer);
    sendJsonUDP(json_buffer);
}


static void read_input(int inputflag, char *pathname) {

    // General
    int sample_rate = 8000;
    unsigned char buffer[4096];
    float fbuf[16384];
    unsigned int fbuf_cnt = 0;
    int i;
    int error;
    int overlap = 0;
    static int integer_only = true;
    int fd = 0;
    unsigned char *sp;
    pa_simple *s;
    pa_sample_spec ss;


    // Fleetsync
    fsync_decoder_t *f_decoder;
    f_decoder = fsync_decoder_new(sample_rate);
    fsync_decoder_set_callback(f_decoder, fsyncCallBack, 0);
    int f_result;

    // MDC1200
    mdc_decoder_t *m_decoder;
    unsigned char op, arg, extra0, extra1, extra2, extra3;
    unsigned short unitID;
    m_decoder = mdc_decoder_new(sample_rate);
    mdc_decoder_set_callback(m_decoder, mdcCallBack, 0);
    int m_result;


    // Pulse init if not reading raw input
    if (inputflag == 0)
        {
        ss.format = PA_SAMPLE_U8;
        ss.channels = 1;
        ss.rate = sample_rate;
    // Try to create the recording stream
        if (!(s = pa_simple_new(NULL, "Fleetsync/MDC1200 Decoder", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
            fprintf(stderr, __FILE__": Pulseaudio Init Failed: %s\n", pa_strerror(error));
            exit(4);
            }
        }

    // Decoder main routine
        fprintf(stderr, "Decoders Initialized\n");
        switch(inputflag)
            {
            case 0:
                fprintf(stderr, "Reading samples from audio input\n");
                break;
            case 1:
                fprintf(stderr, "Reading RAW samples from STDIN\n");
                break;
            case 2:
              fprintf(stderr, "Reading RAW file from pathname\n");
            }

    // Loop over finite file if reading file
    if (inputflag == 2) {
      FILE *fp = fopen(pathname, "r");
      if (!fp) {
        fprintf(stderr, "File open failed\n");
        exit(EXIT_FAILURE);
      }
      while (1 == fread(buffer, sizeof(buffer), 1, fp)) {
        f_result = fsync_decoder_process_samples(f_decoder, buffer, sizeof(buffer));
        m_result = mdc_decoder_process_samples(m_decoder, buffer, sizeof(buffer));
        if (f_result == -1) {
          fprintf(stderr, "Fleetsync Decoder Error\n");
          exit(1);
        }
        if (m_result == -1) {
          fprintf(stderr, "MDC Decoder Error\n");
          exit(1);
        }
      }

    } else if (inputflag < 2) {
      // Loop over input
          for (;;)
              {
              if (inputflag == 0)
                  {
                      if (pa_simple_read(s, buffer, sizeof(buffer), &error) < 0)
                          {
                          fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
                          exit(4);
                          }
                  }

              else
                  {
                      i = read(fd, buffer, sizeof(buffer));
                  }

                      // Magic time
                      // Send buffer to decoders until callback fires
                      // Only care about catching -1 for errors, other return values dont really matter here
                      // Decoders will fire the callbacks when a message is decoded

                      f_result = fsync_decoder_process_samples(f_decoder, buffer, sizeof(buffer));
                      m_result = mdc_decoder_process_samples(m_decoder, buffer, sizeof(buffer));
                      memmove(fbuf, fbuf+fbuf_cnt-overlap, overlap*sizeof(fbuf[0]));
                      fbuf_cnt = overlap;
                      if (f_result == -1)
                          {
                          fprintf(stderr,"Fleetsync Decoder Error\n");
                          exit(1);
                          }
                      if (m_result == -1)
                          {
                          fprintf(stderr,"MDC Decoder Error\n");
                          exit(1);
                          }

              }
    }
}





int main(int argc, char *argv[]) {

    fprintf(stderr, "\n\n\t              Fleetsync / MDC1200 Decoder for Linux                   \n\n");
    fprintf(stderr,     "\t                Run with no   flag for audio input.                   \n");
    fprintf(stderr,     "\t                Run with '-'  flag for STDIN as input.                \n");
    fprintf(stderr,     "\t                Run with '-f' flag for file input.                    \n\n");
    fprintf(stderr,     "\t    STDIN or file input MUST be RAW, MONO 8 bit unsigned integer      \n");
    fprintf(stderr,     "\t                  with a sample rate of 8000hz                        \n\n");
    fprintf(stderr,     "\t      ** NOTE ** - Proper input volume is necessary for decoding      \n");
    fprintf(stderr,     "\t                   if using system audio input                        \n\n");
    fprintf(stderr,     "\t----------------------------------------------------------------------\n\n");
    fprintf(stderr,     "\t  If using SDR (rtlsdr), pipe to SoX using the following settings:    \n");
    fprintf(stderr,     "\t  rtl_fm -f (freq) -s 24000 -p (ppm) -g (gain) |                      \n");
    fprintf(stderr,     "\t  sox -traw -r24000 -e signed-integer -L -b16 -c1 -V1 -v2 - -traw \\  \n");
    fprintf(stderr,     "\t  -e unsigned-integer -b8 -c1 -r8000 - highpass 200 lowpass 4000 |    \n");
    fprintf(stderr,     "\t  ./demod -                                                           \n\n\n");

    // flag 0: audio input (default)
    // flag 1: raw via STDIN
    // flag 2: raw via pathname
    int inputflag = 0;
    char *pathname;

    if (argc == 1) {
      inputflag = 0;
    } else if (argc == 2 && strcmp(argv[1], "-") == 0) {
      inputflag = 1;
    } else if (argc == 3 && strcmp(argv[1], "-f") == 0) {
      inputflag = 2;
      pathname = malloc(strlen(argv[2]) + 1);
      strcpy(pathname, argv[2]);
    } else {
      fprintf(stderr, "Invalid input\n");
      exit(EXIT_FAILURE);
    }

    read_input(inputflag, pathname);


    exit(0);


}
