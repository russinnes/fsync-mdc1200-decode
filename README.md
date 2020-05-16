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

System input audio can be used numerous ways. The decoder will direct sample an audio input connected to a reciever, and gives good results. Another approach is to use existing system audio tools, and SoX, to provide better audio sample capture and filtering/processing. This approach pipes raw packets already processed into the decoder as follows:

	Ex 1: Direct:
		./demod         This does all audio processing internally and works well.

	Ex 2: External Processing using arecord and Sox:
		arecord -r 24000 -t raw -f S16_LE | sox -traw -r24000 -e signed-integer \
		-L -b16 -c1 -V1 -v2 - -traw -e unsigned-integer -b8 -c1 -r8000 - highpass 200 lowpass 4000 | ./demod -


Remember, audio input level will need to be adjusted for best results. Alsamixer and/or pavucontrol can help with this.

	to install:
	chmod a+x build.sh
	./build.sh
	sudo apt-get install sox libpulse-dev
	-optional for fine advanced input control: sudo apt-get install pavucontrol (run in X as needed)

	with rtl-sdr:
	sudo apt-get install rtl-sdr (use your google-fu if needed)

	Executable::
	System audio examples noted above:
		/.demod
	RTL/raw audio (uses Sox for conversion to 8 bit unsigned):
		rtl_fm -s 24000 -g (gain) -p (ppm) -f (frequency) | sox -traw -r24000 -e signed-integer \
		-L -b16 -c1 -V1 -v2 - -traw -e unsigned-integer -b8 -c1 -r8000 - highpass 200 lowpass 4000 | ./demod -




This software contains libraries from:
* [atmatthewat/mdc-encode-decode](https://github.com/atmatthewat/mdc-encode-decode) by Matthew Kaufman
* [EliasOenal/multimon-ng](https://github.com/EliasOenal/multimon-ng) by Elias Önal, Thomas Sailer
