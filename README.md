ay3joderse

VGM file format player for a AY-3-8910/2 chip connected to a Raspberry pi with SPI.
The data is stored in two 74HC595 serial to parallel interfaces.

Play a song:
vgmplay <song>.vgz <-p>

Whith the the optional -p flag the program will generate the clock signal for the
AY-3-8910 with the PWM circuit of the raspberry pi. The signal will be present at pin 12.
Without the flag, an external circuit must be set up to generate the clock signal.

