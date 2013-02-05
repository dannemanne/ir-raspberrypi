#!/usr/bin/pytho 

import time
import RPi.GPIO as GPIO
GPIO.setup(3, GPIO.IN)

result = []
counter = 0
scanning = False
target_time = time.clock()
recording = True

print "Running"

while recording:
    # irinput Default state is True (high)
    irinput = GPIO.input(3)
    
    # If irinput is False then GPIO state is low. That means IR signal is detected
    if irinput == False:
        scanning = True
        counter = 0
        result.append("0")

    # If irinput is True then GPIO state is high. That means we are in between signals.
    else:
        if scanning:
            counter += 1
            result.append("1")
	    
        # If counter has reached 100 then it has been 10ms since state was last "low". Print result
        if counter > 100:
            scanning = False
            recording = False
            print ''.join(result)

    # Sleep will take to long time between cycles to accuratly measure IR pulses. Keep it active instead
    target_time += 0.0006
    while time.clock() < target_time:
        pass

