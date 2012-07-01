# V-Drum MIDI Hi-Hat Drop Clutch / Double Kick

Embedded code for an e-drum addon unit which converts a resistive-based V-Drum hi-hat pedal into a kick pedal, using a MIDI output.

The interesting code is in midi-hh-clutch.c, the rest is support for the [Cyan Technologies](http://www.cyantechnology-ir.com/html/about/company_history.asp) eCog1k eval board I used as the brains of the project.

## Theory of operation

* Take a stereo jack cable from the hi-hat pedal to the throws of a DPDT switch.
* Wire one set of poles of the DPDT to the input of the V-Drum brain to provide normal HH open/close operation to the brain.
* Wire the other set of poles to the eval board ADC input, creating a voltage divider out of the hi-hat pedal and another resistor.
* Connect the MIDI out from the eval board to the V-Drum brain.
* The ADC constantly samples the position of the pedal, and when the position crosses a certain threshold, a single MIDI note is triggered (kick down).
* The note is not triggered again, until the pedal is raised above the threshold, and then depressed again.

## Improvements/TODOs

* Adjust the midi-velocity out to be a function of pedal-down velocity (i.e. rate of change of resistance).
* Replace the eCog1k eval board with something more appropriate, like an MSP430 or an AVR.

## References / Resources

Sending MIDI messages is done simply by soldering some extra components onto the proper socket, and hooking that up to a serial driver and a 5v line.

* [Sending MIDI from an Arduino][1]
* [Wiring a MIDI interface][2]
* [MIDI message reference][3]

[1]: http://itp.nyu.edu/physcomp/Labs/MIDIOutput
[2]: http://www.tigoe.net/pcomp/code/communication/midi/
[3]: http://www.cs.cf.ac.uk/Dave/Multimedia/node158.html#SECTION04133000000000000000