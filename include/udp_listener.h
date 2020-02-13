#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>

#define PORT     8080
#define MAXLINE 1024
#define MAX_SPEED_MSG_TTL 50

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

enum UdpStatus{
    Ok,
    Failed,
};

static int left_wheel_speed_target = 0;
static int right_wheel_speed_target = 0;

static long int total_odometry_left = 0;
static long int total_odometry_right = 0;

unsigned int speed_msg_ttl = MAX_SPEED_MSG_TTL;
static enum UdpStatus udp_status = Ok;


char get_checksum(char* str, int len) {
    int checksum = 0;
    for (int i = 0; i < len; i++){
        checksum ^= str[i];
    }
    return checksum;
}


void* udpListener()
{
    int sockfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr, cliaddr;

    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("Unable to create socket");
        udp_status = Failed;
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if ( bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ) {
        perror("Unable to bind port");
        udp_status = Failed;
        exit(EXIT_FAILURE);
    }

    int n;
    socklen_t len;
    fd_set readfds;

    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    while(true) {
        memset(buffer, 0, MAXLINE * sizeof(buffer[0]));
        select(sockfd+1, &readfds, NULL, NULL, &tv);
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                    &len);

        buffer[n] = '\0';
        if(n > 0) {

            printf("udp received: %s\n", buffer);
            char **arr = NULL;
            unsigned int c = split(buffer, ',', &arr);
            if(strcmp("$spd", arr[0]) == 0) {

                left_wheel_speed_target = MIN(MAX( atoi(arr[1]), -1000), 1000);
                right_wheel_speed_target = MIN(MAX( atoi(arr[2]), -1000), 1000);
                speed_msg_ttl = MAX_SPEED_MSG_TTL;
            } else if(strcmp("$odm", arr[0]) == 0) {
                sprintf(buffer, "$odm,%d,%d", total_odometry_left, total_odometry_right);
                total_odometry_left = 0;
                total_odometry_right = 0;
            }

            printf("udp sending: %s\n", buffer);
            sendto(sockfd, (const char *)buffer, strlen(buffer),
                    MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
                        len);
        } else {

        }



        usleep(10000);
    }
}
