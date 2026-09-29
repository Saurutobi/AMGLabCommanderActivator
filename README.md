# AmgLabCommanderActivator
A set of Arduino apps for use with the AmgLab Commander Timer for use in Practical Competition Shooting sports events.

## Requirements
- An AmgLab Commander
- Arduino with Bluetooth. Arduino Nano 33 BLE Rev2 model used to develop project
- Various microcontroller parts, buttons, relays, powersupplies, etc ***********fill out with stuff

## Projects
### buttonpress_start_withbeep
Hit the button to start the timer.

### buttonpress_start_beepless
Hit the button to start the timer, but it won't make a sound.

### Visual Start
A visual indicator that the Course Of Fire has started. Does not beep the timer.

### Delayed Activator
A mechanism for the Timer to activate the Arduino, which then performs a series of actions in a delayed fashion (like starting a swinger 5s after the beep)

### Others
These projects can be extended to do various actions, they don't need to just need to do the one action they come with. The Visual Start could be mixed with the Delayed Activator, they could activate 3 different actions not just one.