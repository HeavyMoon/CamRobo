#include "PCA9685.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <errno.h>

PCA9685::PCA9685(){
    fd = open("/dev/i2c-1", O_RDWR);
    if(fd < 0){
        perror("open i2c-1 failed");
    }
    ioctl(fd, I2C_SLAVE, PAC9685_ADDR);
    reset();
    setFreq(60);
}

PCA9685::~PCA9685(){
    close(fd);
}

void PCA9685::reset(){
    unsigned char mode1_reset[2] = {MODE1, 0x80};
    write(fd, mode1_reset, sizeof(mode1_reset));
}

void PCA9685::setFreq(int freq){
    unsigned char prescale_value = (CLOCK_FREQ/(4096*freq))-1;
    unsigned char mode1_sleep_on[2] = {MODE1, 0x10};
    unsigned char pre_scale_set[2]  = {PRE_SCALE, prescale_value};
    unsigned char mode1_restart[2]  = {MODE1, 0x80};
    write(fd, mode1_sleep_on, sizeof(mode1_sleep_on));
    write(fd, pre_scale_set,  sizeof(pre_scale_set));
    write(fd, mode1_restart,  sizeof(mode1_restart));
}

int PCA9685::getFreq(){
    unsigned char pre_scale_get[1] = {PRE_SCALE};
    unsigned char pre_scale_val;
    write(fd, pre_scale_get, sizeof(pre_scale_get));
    read(fd, &pre_scale_val, sizeof(pre_scale_val));
    int freq = CLOCK_FREQ/((pre_scale_val-1)*4096);
    return freq;
}

void PCA9685::setPWM(int led, int led_on, int led_off){
    unsigned char led_on_l[2]  = { (unsigned char)(LED0_ON_L  + 4*led), (unsigned char)(led_on & 0xFF)};
    unsigned char led_on_h[2]  = { (unsigned char)(LED0_ON_H  + 4*led), (unsigned char)(led_on >> 8)};
    unsigned char led_off_l[2] = { (unsigned char)(LED0_OFF_L + 4*led), (unsigned char)(led_off & 0xFF)};
    unsigned char led_off_h[2] = { (unsigned char)(LED0_OFF_H + 4*led), (unsigned char)(led_off >> 8)};
    write(fd, led_on_l, sizeof(led_on_l));
    write(fd, led_on_h, sizeof(led_on_h));
    write(fd, led_off_l, sizeof(led_off_l));
    write(fd, led_off_h, sizeof(led_off_h));
}

void PCA9685::setPWM(int  led, int led_off){
    setPWM(led,0,led_off);
}

int PCA9685::getPWM(int led){
    unsigned char led_off_l[1] = {(unsigned char)(LED0_OFF_L + 4*led)};
    unsigned char led_off_l_val;
    unsigned char led_off_h[1] = {(unsigned char)(LED0_OFF_H + 4*led)};
    unsigned char led_off_h_val;
    write(fd, led_off_l,     sizeof(led_off_l));
    read(fd, &led_off_l_val, sizeof(led_off_l_val));
    write(fd, led_off_h,     sizeof(led_off_h));
    read(fd, &led_off_h_val, sizeof(led_off_h_val));
    return (led_off_h_val << 8) + led_off_l_val;
}
