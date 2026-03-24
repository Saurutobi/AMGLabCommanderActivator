Commander uses Nordic UART Service (NUS) to communicate with phone/tablet or whatever bluetooth enabled device you choose to use. This service is free and widely used in the BLE dev. community, many code examples available directly on its developer's (Nordic Semiconductors) gitHub page, as well as on many programming forums on the web.

Service UUID:
6E400001-B5A3-F393-E0A9-E50E24DCCA9E (16-bit offset: 0x0001)
RX Characteristic (6E400002-B5A3-F393-E0A9-E50E24DCCA9E) Write commands for the timer into this characteristic
TX Characteristic (6E400003-B5A3-F393-E0A9-E50E24DCCA9E) Timer sends all data via this characteristic, enable notifications in your BLE app and just monitor incoming data.
The data exchange is happening via data packets, up to 20 bytes long each. Data that is sent to the timer is formatted as simple human readable commands. And data that is coming from the timer can be of two types, first – also formatted as human readable format and second – hexadecimal format, which need to be decoded according to specific rules.

So first, below is the list of commands that you can send to you timer, you can test it without even programming anything, just install nRF Toolbox app, in there go to UART applet and use it connect to the timer and send commands to it, you will see timer responding.

“SET SENSITIVITY XX” (replace xx with number from 01 to 10)

example “SET SENSITIVITY 05”

“SET ECHO DELAY XXX” (xxx is number of hundreds of a second, from 005 to 100, so for example 100 equals to 1.00 second, number 080 equals to 0.8 seond)

“SET BEEP VOLUME X” (x is number from 0 to 3, 0 means beeper off)

“COM START” - starts timer, equivalent of pressing “Go” button on a device

“COM STOP” - stops timer

“SET PRESET X” - switches timer to preset X with X = 1 to 5

"SET DELAY X YYYY ZZZ" – Sets Delay parameters in timer's settings:

X – type of delay, 0 = no delay, 1 = instant, 2 = random

YYYY – fixed delay value

ZZZ – random delay value

"SET PAR TYPE X" – Sets par type in timer's settings:

X = 0, no par time

X = 1, 1 par stop

X = 2, 2 par stops

X = 3, 3 par stops


"SET PAR TIME X YYYYY" – Sets par time value for specific par stop:

X = 1,2 or 3

YYYYY = par time value

These above-mentioned commands will not cause the timer to send anything back, those are “one way” commands so to speak, you just tell the timer what you want to do and it will do it. Please keep in mind that all above mentioned commands (except Start and Stop) need to be sent to the timer when it is stopped, and if you are sending few of those in the row, always provide small delay to make sure timer has enough time to finish processing one command before it can process a next one. 100mS should be sufficient.

Below mentioned commands will trigger a timer to respond and send data back immediately, so once you send any of these commands you should observe Nordic UART TX characteristic for incoming data which can be one or few packets long.

“REQ SCREEN TEXT” - returns current shot information in text format, your UART module will receive 5 packets as ASCII text that will contain all information that timer is currently displaying about the shot in nice human readable format. For example:

“Shot# 5 of 5”

“Time: 3.46”

“Split: 0.33”

“First Shot: 1.24”

“Last Shot: 3.46”

“REQ SCREEN HEX” - returns one packet (12 bytes long) of data that contains all information about current timer display in hex format, it can be parsed directly into variables and is lightning fast so you can poll data from timer without too much strain on BLE, this command is fast, takes only about 5 millisecond of radio time.

Here is how this packet constructed: for example here is the packet you may receive:

"0x01010505015a0021007c015a"

If you look at it from left to right and decode it from hex to decimal it carries following information:

1st byte (0x01) – packet code

2nd byte (0x01) – current timer state i.e. listening for shots or stopped

3rd byte(0x05) – current shot displayed

4th byte (0x05) – total shots

5th and 6th bytes (0x015a) – current shot time in hundreds of second

7th and 8th (0x0021) – split time

9th and 10th byte (0x007c) - first shot

11th and 12th byte (0x015a) - last shot

REQ STRING TEXT – works in similar fashion as req screen text, it will return entire string in readable text format one shot per line, it is time consuming command (because there is a lot of text that needs to be transferred via BLE and BLE exchange data in 20bytes long packets and is not linear, it may take breaks as you phone or tablet can only receive this many packets per session, etc) so if for example you have a string of 50 shots, it may take a quarter of second to transfer via BLE.

REQ STRING HEX – this command will immediately return entire string in hex format, kind of similar to “req screen hex” command except that one always returns single packet, but this command can return different number of packets depending on how many shots your string has as one packet can only carry information about 9 shots and have small header.

Here is an example: let's say you have string of 32 shots currently in timer, when you send “REQ STRING HEX” command you will immediately receive back 4 packets:

Each packet will have first two bytes as a “header” with 1st byte being a packet code and 2nd byte is how many shots this packet is carrying, each shot takes two bytes.

So again, if you have a string of 32 shots and each packet can carry maximum 9 shots, it will take 32/9=3.5 packets to carry entire string with 4th packet filled only partially with data, the header of 4th packet (or specifically 2nd byte of that packet) will tell you when to stop parsing it.

To be even a bit more specific:

Packet#1 will have first two bytes: 0x0A and 0x09, with 0A being a packet code and 09 is how many integers this packet carry

Packet#2 will again have first two bytes as header: 0x0B a packet code and 09 – number of integers this packet carries

Packet#3 will again have two header bytes: 0x0C packet code and again 09 is how many integers this packet carries.

Packet#4 will only carry 5 shots, so this packet's header will be 0x0D – packet code, and 0x05 is how many integers this packet has, so once you parse this packet and extract 5 integers you can stop.

All HEX packets, whether it is a current shot info or entire string info, has the first byte as a packet code, so here is the packet codes table in decimal format.

001 – Currently displayed single shot information

010 – Entire string shots 1 – 9

020 – Entire string shots 10 – 18

030 – Entire string shots 19 –27

040 – Entire string shots 28 – 36

050 – Entire string shots 37 – 45

060 – Entire string shots 46 – 54

070 – Entire string shots 55 – 63

080 – Entire string shots 64 – 72

090 – Entire string shots 73 – 81

100 – Entire string shots 82 – 90

110 – Entire string shots 91 – 99

120 – Entire string shots 100 – 108

130 – Entire string shots 109 – 117

140 – Entire string shots 118 – 126

150 – Entire string shots 127 – 135

160 – Entire string shots 136 – 144

170 – Entire string shots 145 – 150

REQ SETTINGS - will return a HEX packet with currently selected preset settings such as beep volume, par times, delay times, delay type, echo ,etc