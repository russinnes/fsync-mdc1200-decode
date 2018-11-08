# fsync-mdc1200-decode

Linux software-based decoder for Fleetsync 1/2 and MDC1200 radio data signalling formats

A python-based front end provides interfacing to the decoder, however the decoder can be run on its own.
Instructions on usage are commented in the python script.
This software supports both system audio (soundcard input) and RTL-SDR usb sources.
A raw output of UDP packets to localhost also provides an avenue for further scripting.

The following packages need to be installed prior to use:
	
	rtl-sdr
	sox
	libpulse-dev


Run the ./build.sh initially, as it will create a ./demod executable built for your machine architecture.
The included demod executable is built on X86_32.

Not yet working on ARM based processors (buffer overruns) ie : RPi, BPi, etc. 

This software contains libraries from Matthew Kaufman (atmatthewat) and pieces of multimon-ng (EliasOenal, Thomas Sailer)
