#ifndef _PCA9865_H
#define _PCA9685_H

#define PAC9685_ADDR 0x40
#define MODE1        0x00
#define MODE2        0x01
#define LED0_ON_L    0x06
#define LED0_ON_H    0x07
#define LED0_OFF_L   0x08
#define LED0_OFF_H   0x09
#define PRE_SCALE    0xFE
#define CLOCK_FREQ   25000000

class PCA9685 {
public:
    PCA9685();
    ~PCA9685();

    int fd;
    void reset();
    void setFreq(int);
    int  getFreq();
    void setPWM(int,int,int);
    void setPWM(int,int);
    int  getPWM(int);
};
#endif
