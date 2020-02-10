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

#define PORT     8080
#define MAXLINE 1024
#define MAX_SPEED_MSG_TTL 50

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
        select(sockfd+1, &readfds, NULL, NULL, &tv);
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                    MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                    &len);
        if(n > 0) {
            printf("udp received: %s", buffer);
            char **arr = NULL;
            unsigned int c = split(buffer, ',', &arr);
            if(strcmp("$speed", arr[0]) == 0) {
                left_wheel_speed_target = atoi(arr[1]);
                right_wheel_speed_target = atoi(arr[2]);
                speed_msg_ttl = MAX_SPEED_MSG_TTL;
            }
        } else {

        }
        usleep(10000);
    }
}
