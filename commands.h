const String commandNames[] = {"null", "joyUpdate", "writePos", "callPos"};


const byte joyUpdate = 1; // formatting: identifier, joyX, joyY, joyZ, Speed
// ID identifies the call type, in this case a joy update.
// joyX/Y/Z are 0-255, with 127 being centered.
// speed is speed in modes 1-7


const byte writePos = 2; // formatting: identifier, button
const byte callPos = 3; // formatting: identifier, button, speed
// button formatting = none:0, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9, 0:10

// Button formatting = none:0, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9, 0:10, *:11, #:12