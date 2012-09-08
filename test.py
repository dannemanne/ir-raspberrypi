#!/usr/bin/pytho 

import time
import RPi.GPIO as GPIO
GPIO.setup(3, GPIO.IN)

i = 0
while i < 10:
    start_time = time.clock()
    irinput = GPIO.input(3)
    print (time.clock() - start_time).microseconds
    i += 1
