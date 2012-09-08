#!/usr/bin/pytho 

import time
import RPi.GPIO as GPIO
GPIO.setup(3, GPIO.IN)

i = 0
while i < 10:
    start_time = str(time.clock())
    irinput = GPIO.input(3)
    print ''.join([start_time, str(time.clock())])
    i += 1
