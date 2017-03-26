#!/usr/bin/python
'''
This python script acts as an intermediary for the "demod" demodulation program. 
Its main purpose is to integrate the SoX sound engine in-line when using an RTL-SDR
to control signal level and clean bitrate conversions. All received messages are echoed
to the localhost via UDP port 8900 for further scripting if needed.

When using system audio (soundcard) input, start with input level of 1/3 and adjust 
as necessary. The demod program uses Pulse Audio routines for audio sampling and SoX 
is not used when using the system input.

When using an RTL-SDR, you may need to trial-and-error the level offset variable, as well
as the PPM setting of your SDR. Slight differences can make it or break it.
MDC1200 is more forgiving than Fleetsync.
'''
## USER DEFINED VARIABLES ----------------------------------------------------------------

## Uncomment one of these depending on which decoder you want to activate

TYPE = "MDC1200"
#TYPE = "FLEETSYNC"

## Select 0 for system audio, 1 for RTL-SDR
USE_RTL = 1

## Frequency to Tune RTL for decoding (Example: 144.4, 410.8375, etc)
FREQUENCY = 000.000

## PPM Correction for SDR device (Can be found using "rtl_test -p"  - and wait a few minutes)
## Proper PPM is *critical* for decoding
PPM = 5

## * CHANGE TO IMPROVE DECODING * [Affects RTL-SDR ONLY]
## Reciever Signal Offset (range 0.1-2.0) default 1 
## from SDR)-> * change to improve decoding * - A level of 1 is no change. 
LEVEL_OFFSET = 1.25

## END OF USER DEFINED VARIABLES ---------------------------------------------------------

##########################################################################################
#----------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------#
##########################################################################################

import subprocess
import sys
import os
import time
import threading
import thread
import socket
import signal
import traceback
import shlex


class _run_decoder(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.setDaemon(True)
        self.running = 1
        
        ## Local Socket Initializer
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.udp_ip = '127.0.0.1'
        self.udp_port = 8900
        
        ## Decoder Initializer
        decoder_cmd = "rtl_fm -s 24000 -g 40 -p %s -f %s" % (str(PPM), _rtlfreq)
        sox_cmd = "sox -traw -r24000 -es -L -b16 -c1 -V1 -v%s - -traw -es -L -b16 -c1 -r12000 -" % str(LEVEL_OFFSET)
        demod_cmd = "%s/demod %s %s" % (os.getcwd(), _decoder[TYPE], _using_rtl())
        self.decoder_args = shlex.split(decoder_cmd)
        self.sox_args = shlex.split(sox_cmd)
        self.demod_args = shlex.split(demod_cmd)

        if USE_RTL:        
            self.decoder = subprocess.Popen(self.decoder_args,
                                        stdout=subprocess.PIPE, 
                                        stderr=subprocess.PIPE,shell=False) 
            self.sox = subprocess.Popen(self.sox_args,stdin=self.decoder.stdout, 
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE,shell=False)
            self.demod = subprocess.Popen(self.demod_args, stdin=self.sox.stdout, 
                                        stdout=subprocess.PIPE, 
                                        stderr=subprocess.PIPE,shell=False)

        if not USE_RTL:
            self.demod = subprocess.Popen(self.demod_args,
                                        stdout=subprocess.PIPE, 
                                        stderr=subprocess.PIPE,shell=False)

    def run(self):
        time.sleep(2)
        while self.running:
            try:
                line = self.demod.stderr.readline()
                if line:
                    self.udp_send(line)
                    print line.strip()
                    process(line.strip())
                time.sleep(0.1)
            except:
                self.close()
    
    def close(self):
        try:
            self.demod.terminate()
            if USE_RTL:
                self.decoder.terminate()
            self.running = 0
        except:
            traceback.print_exc()
        finally:
            pass
                
    def udp_send(self, line):
        self.socket.sendto(line,(self.udp_ip, self.udp_port))
    
    
def _cmd_exists():
    sox = subprocess.call("type " + "sox", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE) == 0
    rtl_fm = subprocess.call("type " + "rtl_fm", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE) == 0
    demod = subprocess.call("type " + "%s/demod" %os.getcwd(), shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE) == 0
    return sox, rtl_fm, demod

def _check_depends():
    sox , rtl_fm, demod = _cmd_exists()
    print("Checking for Dependencies:.....")
    print("\tSoX: %s" % str(sox))
    print("\tRTL_FM: %s" %str(rtl_fm))
    print("\tdemod: %s" %str(demod))
    if not sox:
        print("\nPlease install SoX (Linux Sound Exchange)\n")
        exit(1)
    if not rtl_fm:
        print("\nPlease install RTL-SDR tools (rtl_fm) \n")
        exit(1)
    if not demod:
        print("\nDemodulator not found!")
        print("Please run the ./build.sh script in this directory to build the demod program!\n")
        exit(1)
    else:
        return 1
        
def _using_rtl():
    if USE_RTL == 1:
        return '-'
    else:
        return ''

def exit_thread(L):
    raw_input()
    L.append(None)
    

_rtlfreq = str(FREQUENCY).replace('.','').ljust(9,'0')
_sox_level = str(LEVEL_OFFSET)
_decoder = {'FLEETSYNC':0,'MDC1200':1}
_check_depends()
_Decoder = _run_decoder()
_Decoder.start()

#----------------------------------------------------------------------------------------#
# FLEETSYNC: 66 252 201 1000 121 1601 1 |testing some more| 17 |?Cf| 30 0 0
def process(input):
    if "FLEETSYNC" in input:
        msg = input.split('|')[1]
        type = input.split('|')[0].split()[0]
        cmd = int(input.split('|')[0].split()[1])
        subcmd = int(input.split('|')[0].split()[2])
        from_flt = int(input.split('|')[0].split()[3])
        from_unit = int(input.split('|')[0].split()[4])
        to_flt = int(input.split('|')[0].split()[5])
        to_unit = int(input.split('|')[0].split()[6])
        allflag = int(input.split('|')[0].split()[7])
        fsync2 = int(input.split('|')[2].split()[3])
        baud = int(input.split('|')[2].split()[4])
        
        print("From: %s%s To: %s%s Msg: %s FS2: %s 2400: %s CMD: %s S_CMD: %s" % \
                (from_flt, from_unit, to_flt, to_unit, msg, fsync2, baud, cmd, subcmd))
    
    
    if "MDC1200" in input:
        if len(input.split(' ')) == 4:
            opcode = input.split(' ')[1]
            arg = input.split(' ')[2]
            unit_id = input.split(' ')[3]
            print("Unit: %s OpCode: %s Arg: %s" % (unit_id, opcode, arg))
        
        if len(input.split(' ')) == 7:
            opcode = input.split(' ')[1]
            arg = input.split(' ')[2]
            unit_id = input.split(' ')[3]
            extra0 = input.split(' ')[4]
            extra1 = input.split(' ')[5]
            extra2 = input.split(' ')[6]
            extra3 = input.split(' ')[7]
            print("Unit: %s OpCode: %s Arg: %s Ex0: %s Ex1: %s Ex2: %s Ex3: %s" % \
                (unit_id, opcode, arg, extra0, extra1, extra2, extra3))
        
#----------------------------------------------------------------------------------------#



L = []
thread.start_new_thread(exit_thread, (L,))
print("Press Enter to Exit\n")
while 1:
    time.sleep(0.5)
    if L:
        print("Exiting....")
        _Decoder.close()
        _Decoder.join()
        time.sleep(5)
        exit()
        
 