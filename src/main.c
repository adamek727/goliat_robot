#include <stdio.h>
#include <stdlib.h>

#include "i2c.h"
#include "km2.h"
#include "split.h"
#include "udp_listener.h"

#define RAMP_CONSTANT 0.2
#define DEBUG

void debug_printout();

static int left_wheel_speed = 0;
static int right_wheel_speed = 0;
static int odometry_left = 0;
static int odometry_right = 0;

int main(int argc, char* argv[]) {
		
    int fd = i2c_init(1);

    pthread_t udpListener_tid;
    pthread_create(&udpListener_tid, NULL, udpListener, NULL);

    while(1) {

        if(speed_msg_ttl > 0) {
            left_wheel_speed += (left_wheel_speed_target-left_wheel_speed) * RAMP_CONSTANT;
            right_wheel_speed += (right_wheel_speed_target-right_wheel_speed) * RAMP_CONSTANT;
            speed_msg_ttl--;
        } else {
            left_wheel_speed_target = right_wheel_speed_target = 0;
            left_wheel_speed = right_wheel_speed = 0;
        }

        usleep(10000);
        km2_drive(fd, 0x71, (int16_t)left_wheel_speed, (int16_t)right_wheel_speed);
        usleep(10000);
        km2_odometry(fd, 0x71, &odometry_left, &odometry_right);
        usleep(10000);

        total_odometry_left += odometry_left;
        total_odometry_right += odometry_right;

#ifdef DEBUG
        debug_printout();
#endif
        if(udp_status == Failed) {
            printf("UDP Failed ... exiting!");
            break;
        }
        usleep(100000); // 100 ms
    }

    i2c_close(fd);
    return 0;
}



void debug_printout() {
    printf(" * * * * * * * * * *\n");
    printf("targets-> l: %d r: %d\n", left_wheel_speed_target, right_wheel_speed_target);
    printf("real-> l: %d r: %d\n", left_wheel_speed, right_wheel_speed);
    printf("speed msg ttl: %d\n", speed_msg_ttl);
    printf("odometry l: %d r: %d\n", odometry_left, odometry_right);
    printf("total odometry l: %ld r: %ld\n", total_odometry_left, total_odometry_right);
}


