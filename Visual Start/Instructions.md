# Visual Start
Upon activation, will wait 1-4 seconds before activating the mechanism and simultaneously starting the timer.

## Example build
This build Lights a 5V LED for 2 seconds as it's visual mechanism.

### Requirements
- An AMGLab Commander
- Arduino with Bluetooth. *********************** model used to develop project
- 5V LED
- 5V Button
- wires, case, powersupply.

### How It Works
1. The RO gives the Make Ready/Are You Ready.
2. The RO gives the Standy command and at the same time presses the start button
3. The Arduino connects and sends the mute command to the Commander
4. 1-4 seconds after the button is pressed, the Arduino sets the pin used for the light to high and sends the start command to the Commander

### Troubleshooting
You will see the Commander reset and wait for shots as if the Go button is pressed when the Start command is sent to it. Should the Commander not show signs of running, something went wrong with the Arduino connection. Turn both devices off and then back on again, try again.