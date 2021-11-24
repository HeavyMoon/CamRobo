CamRobo
=======================================
## DESCRIPTION
CamRobo remote control program using TCP/IP.  
the commander program occupies the keyboard.
these are test codes, so change the IP address and device according to your environment.

[check the hardware components here](https://heavymoon.org/2021/11/05/remote-camrobo-1/)
and here is [Original R2D2 Sound Generator Project](https://www.instructables.com/R2D2-Sound-Generator/)

## SETUP CamRobo Agent
1. Install requires
```
$ sudo apt-get install python3-rpi.gpio python3-fasteners
```
2. Clone
```
$ cd /opt
$ git clone https://github.com/HeavyMoon/CamRobo
```
3. Compile
```
$ cd CamRobo
$ g++ -o CamRoboAgent CamRoboAgent.cpp PCA9685.cpp
```
4. Execute
```
$ g++ -o CamRoboAgent CamRoboAgent.cpp PCA9685.cpp
```

## SETUP CamRobo Commander
Note: change the IP address and device according to your environment.

1. Clone
```
$ git clone https://github.com/HeavyMoon/CamRobo
```
2. Compile
```
$ cd CamRobo
$ g++ -o CamRoboCommander CamRoboCommander.cpp
```
3. Execute
```
$ sudo ./CamRoboCommander
```

