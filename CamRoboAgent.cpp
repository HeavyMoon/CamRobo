// ----------------------------------------------------- //
// CAM ROBO AGENT
// ----------------------------------------------------- //
// compile
//  $ g++ -o CamRoboAgent CamRoboAgent.cpp PCA9685.cpp
// NOTE
//  Change the script path according to your environment.
// ----------------------------------------------------- //

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "PCA9685.h"

#define _VERSION_   "1.0 beta"

#define _CMD_SPEAK_R2D2_        "/usr/bin/python3 /home/pi/CamRobo/CamRoboR2D2.py &"
#define _CMD_FFMPEG_STREAMING_  "/usr/bin/bash /home/pi/CamRobo/ffmpeg_streaming.sh &"

// PCA9685 pin assign
#define P_ARM_R      0
#define P_ARM_L      1
#define P_CAM        2
#define P_LED_B      5
#define P_LED_G      6
#define P_LED_R      7
#define P_MOTOR_L1   8
#define P_MOTOR_L2   9
#define P_MOTOR_R1  10
#define P_MOTOR_R2  11
#define P_DC_FAN    12

// PWM value
#define PWM_MAX             4095
#define PWM_MIN             0
#define ARM_L_LIMIT_UP      110
#define ARM_L_LIMIT_DOWN    600
#define ARM_R_LIMIT_UP      600
#define ARM_R_LIMIT_DOWN    110
#define ARM_PWM_RANGE       490         
#define CAM_LIMIT_MIN       150
#define CAM_LIMIT_MAX       510
#define CAM_PWM_RANGE       360

void log_msg(const char *log_level, const char *format, ...) {
    time_t timer;
    struct tm *local;
    timer = time(NULL);
    local = localtime(&timer);

    int year    = local->tm_year + 1900;
    int month   = local->tm_mon + 1;
    int day     = local->tm_mday;
    int hour    = local->tm_hour;
    int minute  = local->tm_min;
    int second  = local->tm_sec;

    if(strcmp(log_level,"WARNING")==0){
        printf("\x1b[33m");                 // text to yellow
    }
    if(strcmp(log_level,"ERROR")==0){
        printf("\x1b[31m");                 // text to red
    }
    if(strcmp(log_level,"CRITICAL")==0){
        printf("\x1b[41m");                 // bg to red
    }

    printf("%d-%02d-%02d %02d:%02d:%02d [%s] ",year,month,day,hour,minute,second,log_level);
    va_list args;
    va_start(args,format);
    vfprintf(stdout,format,args);
    va_end(args);
    printf("\n");
    printf("\x1b[39m"); // reset text color
    printf("\x1b[49m"); // reset bg color
}

void banner() {
    printf("\x1b[36m");     // text to cyaan
    printf("      ___           ___           ___                    ___           ___           ___           ___     \n");
    printf("     /\\  \\         /\\  \\         /\\__\\                  /\\  \\         /\\  \\         /\\  \\         /\\  \\    \n");
    printf("    /::\\  \\       /::\\  \\       /::|  |                /::\\  \\       /::\\  \\       /::\\  \\       /::\\  \\   \n");
    printf("   /:/\\:\\  \\     /:/\\:\\  \\     /:|:|  |               /:/\\:\\  \\     /:/\\:\\  \\     /:/\\:\\  \\     /:/\\:\\  \\  \n");
    printf("  /:/  \\:\\  \\   /::\\~\\:\\  \\   /:/|:|__|__            /::\\~\\:\\  \\   /:/  \\:\\  \\   /::\\~\\:\\__\\   /:/  \\:\\  \\ \n");
    printf(" /:/__/ \\:\\__\\ /:/\\:\\ \\:\\__\\ /:/ |::::\\__\\          /:/\\:\\ \\:\\__\\ /:/__/ \\:\\__\\ /:/\\:\\ \\:|__| /:/__/ \\:\\__\\\n");
    printf(" \\:\\  \\  \\/__/ \\/__\\:\\/:/  / \\/__/~~/:/  /          \\/_|::\\/:/  / \\:\\  \\ /:/  / \\:\\~\\:\\/:/  / \\:\\  \\ /:/  /\n");
    printf("  \\:\\  \\            \\::/  /        /:/  /              |:|::/  /   \\:\\  /:/  /   \\:\\ \\::/  /   \\:\\  /:/  / \n");
    printf("   \\:\\  \\           /:/  /        /:/  /               |:|\\/__/     \\:\\/:/  /     \\:\\/:/  /     \\:\\/:/  /  \n");
    printf("    \\:\\__\\         /:/  /        /:/  /                |:|  |        \\::/  /       \\::/__/       \\::/  /   \n");
    printf("     \\/__/         \\/__/         \\/__/                  \\|__|         \\/__/         ~~            \\/__/    \n");
    printf("                                                                                                 AGENT v%s\n",_VERSION_);
    printf("\x1b[39m");     // reset text color
}

// --------------------------
//  cal_arm_pwm
//  args
//      const char *LR  : "LEFT", "RIGHT"
//      int current_pmw : 0 ~ 4095
//      double diff     : -1.0(down) ~ 1.0(up)
//  return
//      int pwm         : 110 ~ 600
// --------------------------
int cal_arm_pwm(const char *LR, int current_pwm, double diff) {
    int pwm = current_pwm;
    if(strcmp(LR,"LEFT")==0){   // 600(down) - 110(up)
        pwm += int(ARM_PWM_RANGE * -diff);
        if(pwm < ARM_L_LIMIT_UP){
            pwm = ARM_L_LIMIT_UP;
        }
        if(pwm > ARM_L_LIMIT_DOWN){
            pwm = ARM_L_LIMIT_DOWN;
        }
    }
    if(strcmp(LR,"RIGHT")==0){  // 110(down) - 600(up)
        pwm += int(ARM_PWM_RANGE * diff);
        if(pwm > ARM_R_LIMIT_UP){
            pwm = ARM_R_LIMIT_UP;
        }
        if(pwm < ARM_R_LIMIT_DOWN){
            pwm = ARM_R_LIMIT_DOWN;
        }
    }
    return pwm;
}

// --------------------------
//  cal_cam_pwm
//  args
//      int current_pmw : 0 ~ 4095
//      double diff     : -1.0(down) ~ 1.0(up)
//  return
//      int pwm         : 150 ~ 510
// --------------------------
int cal_cam_pwm(int current_pwm, double diff) {
    int pwm = current_pwm;
    pwm += int(CAM_PWM_RANGE * diff);
    if(pwm < CAM_LIMIT_MIN){
        pwm = CAM_LIMIT_MIN;
    }
    if(pwm > CAM_LIMIT_MAX){
        pwm = CAM_LIMIT_MAX;
    }
    return pwm;
}

int main() {
    banner();
    log_msg("INFO","start cam robo agent.");

    int rc;

    log_msg("INFO","initializing PCA9685...");
    PCA9685 pca;
    pca.setFreq(50); 
    pca.setPWM(P_ARM_R,ARM_R_LIMIT_UP);
    pca.setPWM(P_ARM_L,ARM_L_LIMIT_UP);
    pca.setPWM(P_CAM,CAM_LIMIT_MIN);
    pca.setPWM(P_LED_B,0);
    pca.setPWM(P_LED_G,0);
    pca.setPWM(P_LED_R,0);
    pca.setPWM(P_MOTOR_L1,0);
    pca.setPWM(P_MOTOR_L2,0);
    pca.setPWM(P_MOTOR_R1,0);
    pca.setPWM(P_MOTOR_R2,0);

    double gear[5] = {0.0, 0.3, 0.5, 0.7, 1.0};
    int gear_position = 1;
    int cam_flag = 1;

    log_msg("INFO","initializing acceptor socket...");
    int accSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in accAddr;
    accAddr.sin_family      = AF_INET;
    accAddr.sin_port        = htons(12345);
    accAddr.sin_addr.s_addr = INADDR_ANY;

    log_msg("INFO","start bind");
    rc = bind(accSocket, (struct sockaddr*)&accAddr, sizeof(accAddr));
    if(rc<0){
        perror("bind() failed");
        exit(1);
    }

    log_msg("INFO","start listen at %s:%d",inet_ntoa(accAddr.sin_addr),ntohs(accAddr.sin_port));
    rc = listen(accSocket,5);
    if(rc<0){
        perror("listen() failed");
        exit(1);
    }

    log_msg("INFO","initializing connector socket...");
    int conSocket;
    struct sockaddr_in conAddr;

    log_msg("INFO","start main service.");
    while(1){
        log_msg("INFO","waiting for accept");
        int conSocketLen = sizeof(conAddr);
        conSocket = accept(accSocket, (struct sockaddr*)&conAddr, (socklen_t*)&conSocketLen);
        if(conSocket<0){
            perror("accept() failed");
            exit(1);
        }
        log_msg("INFO","accepted connection from %s:%d", inet_ntoa(conAddr.sin_addr), ntohs(conAddr.sin_port));
        pca.setPWM(P_LED_R,int(PWM_MAX *0.1));
        pca.setPWM(P_LED_G,int(PWM_MAX *0.1));
        pca.setPWM(P_LED_B,int(PWM_MAX *0.1));
        system(_CMD_SPEAK_R2D2_);

        log_msg("INFO","initializing poll fds...");
        struct pollfd fds[1];
        memset(&fds, 0, sizeof(fds));
        fds[0].fd = conSocket;
        fds[0].events = POLLIN | POLLERR | POLLHUP | POLLRDHUP;

        while(1){
            rc = poll(fds,1,-1);

            if(fds[0].revents & POLLRDHUP){
                log_msg("INFO","close connection by POLLRDHUP.");
                close(fds[0].fd);
                break;
            }
            if(fds[0].revents & POLLERR){
                log_msg("WARNING","close connection by POLLERR.");
                close(fds[0].fd);
                break;
            }
            if(fds[0].revents & POLLHUP){
                log_msg("WARNING","close connection by POLLHUP.");
                close(fds[0].fd);
                break;
            }

            if(fds[0].revents & POLLIN) {
                char tmpSockBuf[1024];
                memset(tmpSockBuf,'\0',sizeof(tmpSockBuf));
                rc = read(conSocket,tmpSockBuf,sizeof(tmpSockBuf));
                if(rc<0){
                    perror("read failed");
                }

                char *cmdSet[10];
                int   cnt=0;
                cmdSet[cnt++] = strtok(tmpSockBuf,";");
                while(cmdSet[cnt] = strtok(NULL,";")){
                    cnt++;
                }

                for(int i=0;i<cnt;i++){
                    log_msg("INFO","recieved command set: %s", cmdSet[i]);

                    char *cmd;
                    char *tmpOpt;
                    int opt[3];
                    int j=0;
                    cmd = strtok(cmdSet[i],":");
                    while(tmpOpt = strtok(NULL,":")){
                        opt[j++] = atoi(tmpOpt);
                    }

                    // SPEAK
                    if(strcmp(cmd,"SPK")==0){
                        system(_CMD_SPEAK_R2D2_);
                    }
                    // Crawler Left Forward
                    if(strcmp(cmd,"CLF")==0){
                        switch(opt[0]){
                            case 0:
                                pca.setPWM(P_MOTOR_L1,int(PWM_MAX * gear[0]));
                                break;
                            case 1:
                                pca.setPWM(P_MOTOR_L1,int(PWM_MAX * gear[gear_position]));
                                break;
                            case 2:
                                break;
                        }
                    }
                    // Crawler Left Back
                    if(strcmp(cmd,"CLB")==0){
                        switch(opt[0]){
                            case 0:
                                pca.setPWM(P_MOTOR_L2,int(PWM_MAX * gear[0]));
                                break;
                            case 1:
                                pca.setPWM(P_MOTOR_L2,int(PWM_MAX * gear[gear_position]));
                                break;
                            case 2:
                                break;
                        }
                    }
                    // Crawler Right Forward
                    if(strcmp(cmd,"CRF")==0){
                        switch(opt[0]){
                            case 0:
                                pca.setPWM(P_MOTOR_R1,int(PWM_MAX * gear[0]));
                                break;
                            case 1:
                                pca.setPWM(P_MOTOR_R1,int(PWM_MAX * gear[gear_position]));
                                break;
                            case 2:
                                break;
                        }
                    }
                    // Crawler Right Back
                    if(strcmp(cmd,"CRB")==0){
                        switch(opt[0]){
                            case 0:
                                pca.setPWM(P_MOTOR_R2,int(PWM_MAX * gear[0]));
                                break;
                            case 1:
                                pca.setPWM(P_MOTOR_R2,int(PWM_MAX * gear[gear_position]));
                                break;
                            case 2:
                                break;
                        }
                    }
                    // Crawler Gear Position
                    if(strcmp(cmd,"CGR")==0){
                        gear_position = opt[0];
                    }
                    // Arm Left Up
                    if(strcmp(cmd,"ALU")==0){
                        int pwm;
                        switch(opt[0]){
                            case 0:
                                break;
                            case 1:
                                pwm = cal_arm_pwm("LEFT", pca.getPWM(P_ARM_L), 0.1);    // +10% UP
                                pca.setPWM(P_ARM_L,pwm);
                                break;
                            case 2:
                                pwm = cal_arm_pwm("LEFT", pca.getPWM(P_ARM_L), 0.2);    // +20% UP
                                pca.setPWM(P_ARM_L,pwm);
                                break;
                        }
                    }
                    // Arm Left Fown
                    if(strcmp(cmd,"ALD")==0){
                        int pwm;
                        switch(opt[0]){
                            case 0:
                                break;
                            case 1:
                                pwm = cal_arm_pwm("LEFT", pca.getPWM(P_ARM_L), -0.1);    // -10% DOWN
                                pca.setPWM(P_ARM_L,pwm);
                                break;
                            case 2:
                                pwm = cal_arm_pwm("LEFT", pca.getPWM(P_ARM_L), -0.2);    // -20% DOWN
                                pca.setPWM(P_ARM_L,pwm);
                                break;
                        }
                    }
                    // Arm Right Up
                    if(strcmp(cmd,"ARU")==0){
                        int pwm;
                        switch(opt[0]){
                            case 0:
                                break;
                            case 1:
                                pwm = cal_arm_pwm("RIGHT", pca.getPWM(P_ARM_R), 0.1);    // +10% UP
                                pca.setPWM(P_ARM_R,pwm);
                                break;
                            case 2:
                                pwm = cal_arm_pwm("RIGHT", pca.getPWM(P_ARM_R), 0.2);    // +20% UP
                                pca.setPWM(P_ARM_R,pwm);
                                break;
                        }
                    }
                    // Arm Right Down
                    if(strcmp(cmd,"ARD")==0){
                        int pwm;
                        switch(opt[0]){
                            case 0:
                                break;
                            case 1:
                                pwm = cal_arm_pwm("RIGHT", pca.getPWM(P_ARM_R), -0.1);    // -10% DOWN
                                pca.setPWM(P_ARM_R,pwm);
                                break;
                            case 2:
                                pwm = cal_arm_pwm("RIGHT", pca.getPWM(P_ARM_R), -0.2);    // -20% DOWN
                                pca.setPWM(P_ARM_R,pwm);
                                break;
                        }
                    }
                    // LED
                    if(strcmp(cmd,"LED")==0){
                        int r = int(PWM_MAX * opt[0] / 100.0);
                        int g = int(PWM_MAX * opt[1] / 100.0);
                        int b = int(PWM_MAX * opt[2] / 100.0);
                        pca.setPWM(P_LED_R,r);
                        pca.setPWM(P_LED_G,g);
                        pca.setPWM(P_LED_B,b);
                    }
                    // Camera Angle Up
                    if(strcmp(cmd,"CAU")==0){
                        int pwm;
                        switch(opt[0]){
                            case 0:
                                break;
                            case 1:
                                pwm = cal_cam_pwm(pca.getPWM(P_CAM), 0.1);
                                pca.setPWM(P_CAM,pwm);
                                break;
                            case 2:
                                pwm = cal_cam_pwm(pca.getPWM(P_CAM), 0.2);
                                pca.setPWM(P_CAM,pwm);
                                break;
                        }
                    }
                    // Camera Angle Down
                    if(strcmp(cmd,"CAD")==0){
                        int pwm;
                        switch(opt[0]){
                            case 0:
                                break;
                            case 1:
                                pwm = cal_cam_pwm(pca.getPWM(P_CAM), -0.1);
                                pca.setPWM(P_CAM,pwm);
                                break;
                            case 2:
                                pwm = cal_cam_pwm(pca.getPWM(P_CAM), -0.2);
                                pca.setPWM(P_CAM,pwm);
                                break;
                        }
                    }
                    // Camera Streaming
                    if(strcmp(cmd,"CST")==0){
                        switch(opt[0]){
                            case 0:
                                // TODO: terminate ffmpeg camera streaming
                                break;
                            case 1:
                                // TODO: launch ffmpeg camera streaming
                                // system(_CMD_FFMPEG_STREAMING_);
                                break;
                            case 2:
                                break;
                        }
                    }
                }
            }
        }

        log_msg("INFO","reset PCA9685.");
        pca.setPWM(P_ARM_R,ARM_R_LIMIT_UP);
        pca.setPWM(P_ARM_L,ARM_L_LIMIT_UP);
        pca.setPWM(P_CAM,CAM_LIMIT_MIN);
        pca.setPWM(P_LED_B,0);
        pca.setPWM(P_LED_G,0);
        pca.setPWM(P_LED_R,0);
        pca.setPWM(P_MOTOR_L1,0);
        pca.setPWM(P_MOTOR_L2,0);
        pca.setPWM(P_MOTOR_R1,0);
        pca.setPWM(P_MOTOR_R2,0);
        gear_position = 1;
    }

    // never reach here
    log_msg("INFO","terminate cam robo agent.");
    close(accSocket);
    return 0;
}
