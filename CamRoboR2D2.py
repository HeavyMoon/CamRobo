#!/usr/bin/python3
####################################################
#  Project R2D2 Sound Generator
#  
#  To all fans of StarWars and Arduino!
#
#  Written by Marcelo Larios
#  
#  BSD license, all text above must be included in any redistribution
####################################################

####################################################
# converted by HeavyMoon
# reference: https://www.instructables.com/R2D2-Sound-Generator/
#
# require pkg
#   python3-rpi.gpio
#   python3-fasteners
#
####################################################

import RPi.GPIO as GPIO
import time
import sys
import random
import fasteners

# INIT
lock = fasteners.InterProcessLock('/tmp/spk_lockfile')
if not lock.acquire(blocking=False):
    exit()

GPIO.setmode(GPIO.BCM)
pwm_pin = 12
GPIO.setup(pwm_pin, GPIO.OUT)
pwm = GPIO.PWM(pwm_pin, 50)    # Duty 50%

def phrase1():
    k = random.randint(1000,2000)
    for i in range(0,random.randint(100,int(k/2)),12):
        pwm.ChangeFrequency(k+(-i*2))
        pwm.start(50)
        time.sleep(random.uniform(0.0009,0.002))
        pwm.stop()
    for i in range(0,random.randint(100,1000),12):
        pwm.ChangeFrequency(k+(i*10))
        pwm.start(50)
        time.sleep(random.uniform(0.0009,0.002))
        pwm.stop()

def phrase2():
    k = random.randint(1000,2000)
    for i in range(0,random.randint(100,2000),12):
        pwm.ChangeFrequency(k+(i*2))
        pwm.start(50)
        time.sleep(random.uniform(0.0009,0.002))
        pwm.stop()
    for i in range(0,random.randint(100,int(k/10)),12):
        pwm.ChangeFrequency(k+(-i*10))
        pwm.start(50)
        time.sleep(random.uniform(0.0009,0.002))
        pwm.stop()

# MAIN
K = 2000
r = random.randint(1,6)
if   r == 1:
    phrase1()
elif r == 2:
    phrase2()
elif r == 3:
    phrase1()
    phrase2()
elif r == 4:
    phrase2()
    phrase1()

for i in range(0,random.randint(3,9)):
    pwm.ChangeFrequency(K+random.randint(-1700,2000))
    pwm.start(50)
    time.sleep(random.uniform(0.07,0.170))
    pwm.stop()
    time.sleep(random.uniform(0.0,0.03))

pwm.stop()
GPIO.cleanup()
lock.release()
