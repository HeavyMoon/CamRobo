// ----------------------------------------------------- //
// CAM ROBO COMMANDER
// ----------------------------------------------------- //
// compile
//  $ g++ -o CamRoboCommander CamRoboCommander.cpp
// exec
//  $ sudo ./CamRoboCommander
// NOTE
//  Change the IP address and device according to your environment.
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
#include <fcntl.h>
#include <linux/input.h>

#define _VERSION_       "1.0 beta"

#define _RoboCam_IP_     "192.168.1.101"
#define _Keyboard_Dev_   "/dev/input/event1"

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
    printf("                                                                                              COMMANDER v%s\n",_VERSION_);
    printf("\x1b[39m");     // reset text color
}

void cmd_list() {
    printf("\n");
    printf("-- command list --\n");
    printf("service command\n");
    printf("    esc     terminate commander.\n");
    printf("control command\n");
    printf("    JF      Crawler Forward\n");
    printf("    DK      Crawler Back\n");
    printf("    ZXCV    Gear Position\n");
    printf("    SL      Arm UP\n");
    printf("    A;      Arm Down\n");
    printf("    QWERT   LET ON/OFF\n");
    printf("    space   Speak\n");
    printf("\n");
}

int main(void) {
    banner();
    log_msg("INFO","start cam robo commander.");
    cmd_list();
    sleep(1);   // wait for release enter key

    int rc;

    log_msg("INFO","initializing keyboard fd...");
    struct input_event kbd_event;
    int kbdFd = open(_Keyboard_Dev_, O_RDONLY | O_NONBLOCK);
    ioctl(kbdFd, EVIOCGRAB, 1);

    log_msg("INFO","initializing connector socket...");
    struct sockaddr_in conAddr;
    int conSocket = socket(AF_INET, SOCK_STREAM, 0);
    conAddr.sin_family         = AF_INET;
    conAddr.sin_port           = htons(12345);
    conAddr.sin_addr.s_addr    = inet_addr(_RoboCam_IP_);
    rc = connect(conSocket, (struct sockaddr*)&conAddr, sizeof(conAddr));
    if(rc<0){
        perror("connect() failed");
        exit(1);
    }

    log_msg("INFO","initializing poll fds...");
    struct pollfd fds[2];
    memset(&fds, 0, sizeof(fds));
    fds[0].fd       = kbdFd;
    fds[0].events   = POLLIN | POLLERR;
    fds[1].fd       = conSocket;
    fds[1].events   = POLLIN | POLLERR;
    int nfds = sizeof(fds)/sizeof(struct pollfd);

    log_msg("INFO","start main service.");
    while(1) {
        poll(fds,nfds,-1);

        if(fds[0].revents & POLLERR){
            log_msg("WARNING","close connection by POLLERR.");
            close(fds[0].fd);
            break;
        }
        if(fds[0].revents & POLLIN) {
            read(fds[0].fd, &kbd_event, sizeof(kbd_event));

            if(kbd_event.type == EV_KEY) {
                const char *cmd;
                // terminate commander service
                if(kbd_event.code == KEY_ESC){
                    break;
                }

                // Speak
                if(kbd_event.code == KEY_SPACE){

                    switch(kbd_event.value){
                        case 0:     // release
                            // speak
                            cmd = "SPK;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 1:     // press
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                // Crawler Left Forward
                if(kbd_event.code == KEY_F){
                    switch(kbd_event.value){
                        case 0:     // release
                            cmd = "CLF:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 1:     // press
                            cmd = "CLF:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                // Crawler Left Back
                if(kbd_event.code == KEY_D){
                    switch(kbd_event.value){
                        case 0:     // release
                            cmd = "CLB:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 1:     // press
                            cmd = "CLB:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                // Crawler Right Forward
                if(kbd_event.code == KEY_J){
                    switch(kbd_event.value){
                        case 0:     // release
                            cmd = "CRF:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 1:     // press
                            cmd = "CRF:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                // Crawler Right Back
                if(kbd_event.code == KEY_K){
                    switch(kbd_event.value){
                        case 0:     // release
                            cmd = "CRB:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 1:     // press
                            cmd = "CRB:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }

                // Crawler Gear Position 1
                if(kbd_event.code == KEY_Z){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "CGR:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                // Crawler Gear Position 2
                if(kbd_event.code == KEY_X){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "CGR:2;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                // Crawler Gear Position 3
                if(kbd_event.code == KEY_C){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "CGR:3;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                // Crawler Gear Position 4
                if(kbd_event.code == KEY_V){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "CGR:4;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                // Arm Left Up
                if(kbd_event.code == KEY_S){
                    switch(kbd_event.value){
                        case 0:     // release
                            cmd = "ALU:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 1:     // press
                            cmd = "ALU:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            cmd = "ALU:2;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                    }
                }
                // Arm Left Down
                if(kbd_event.code == KEY_A){
                    switch(kbd_event.value){
                        case 0:     // release
                            cmd = "ALD:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 1:     // press
                            cmd = "ALD:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            cmd = "ALD:2;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                    }
                }
                // Arm Right Up
                if(kbd_event.code == KEY_L){
                    switch(kbd_event.value){
                        case 0:     // release
                            cmd = "ARU:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 1:     // press
                            cmd = "ARU:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            cmd = "ARU:2;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                    }
                }
                // Arm Right Down
                if(kbd_event.code == KEY_SEMICOLON){
                    switch(kbd_event.value){
                        case 0:     // release
                            cmd = "ARD:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 1:     // press
                            cmd = "ARD:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            cmd = "ARD:2;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                    }
                }
                // LED OFF
                if(kbd_event.code == KEY_Q){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "LED:0:0:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                // LED color to White
                if(kbd_event.code == KEY_W){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "LED:10:10:10;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            cmd = "LED:100:100:100;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                    }
                }
                // LED color to Red
                if(kbd_event.code == KEY_E){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "LED:10:0:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            cmd = "LED:100:0:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                    }
                }
                // LED color to Green
                if(kbd_event.code == KEY_R){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "LED:0:10:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            cmd = "LED:0:100:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                    }
                }
                // LED color to Blue
                if(kbd_event.code == KEY_T){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "LED:0:0:10;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            cmd = "LED:0:0:100;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                    }
                }
                // Camera Angle Up
                if(kbd_event.code == KEY_M){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "CAU:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            cmd = "CAU:2;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                    }
                }
                // Camera Angle Down
                if(kbd_event.code == KEY_COMMA){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "CAD:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            cmd = "CAD:2;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                    }
                }
                // Camera Streaming Start
                if(kbd_event.code == KEY_DOT){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "CST:1;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                // Camera Streaming Stop
                if(kbd_event.code == KEY_SLASH){
                    switch(kbd_event.value){
                        case 0:     // release
                            break;
                        case 1:     // press
                            cmd = "CST:0;";
                            write(conSocket,cmd,strlen(cmd));
                            break;
                        case 2:     // long press
                            break;
                    }
                }
                log_msg("INFO","sent command: %s",cmd);
            }
        }

        // recieve socket event
        if(fds[1].revents & POLLIN) {
            char buf[1024];
            memset(buf, 0, sizeof(buf));
            read(conSocket, buf, sizeof(buf));
            log_msg("INFO","recieved sock event: %s", buf);
        }
    }

    log_msg("INFO","terminate commander service.");
    ioctl(kbdFd, EVIOCGRAB, 0);
    for(int i=0; i<nfds; i++){
        close(fds[i].fd);
    }
    return 0;
}
