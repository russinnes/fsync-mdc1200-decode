# fsync-mdc1200-decode

Linux software-based decoder for Fleetsync 1,2 and MDC1200 radio data signalling formats

Major overhaul from the last version!

The following packages need to be installed prior to use:
	
	rtl-sdr (if used as a signal source)
	sox
	libpulse-dev


Run the ./build.sh initially, as it will create a ./demod executable built for your machine architecture.

Simultaneously decodes Kenwood Fleetsync and Motorola MCD-1200. Output is JSON formatted, and also echoed to 
UDP port 9101 on localhost/127.0.0.1. Output JSON is to STDOUT to allow further piping as required, all other 
output is to STDERR. 

Once compiled using ./build.sh, execute the ./demod executable to start the program. Default source uses
system audio input. To read from STDIN (for instance, from rtlsdr) use the '-' flag (./demod -)

System audio requires PulseAudio and SoX, raw input requires Sox at minumum for bitrate conversion. Adding the HpLp filter
will help keep noise out of the decoder, as all data is modulated below 3000Hz.

to install:
./build.sh
sudo apt-get install sox libpulse-dev 
	-optional for fine advanced input control: sudo apt-get install pavucontrol (run in X as needed)

with rtl-sdr:
sudo apt-get install rtl-sdr (use your google-fu if needed)

Executable:: 
System audio: 
	/.demod
RTL/raw audio (uses Sox for conversion to 8 bit unsigned):
	rtl_fm -s 24000 -g (gain) -p (ppm) -f (frequency) | sox -traw -r24000 -e signed-integer -L -b16 -c1 -V1 -v2 - \
	-traw -e unsigned-integer -b8 -c1 -r8000 - highpass 200 lowpass 4000 | ./demod -




This software contains libraries from Matthew Kaufman (atmatthewat) and pieces of multimon-ng (EliasOenal, Thomas Sailer)
