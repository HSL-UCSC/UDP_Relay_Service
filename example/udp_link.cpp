/*****************************************************************
 Copyright (c) 2020, Unitree Robotics.Co.Ltd. All rights reserved.
******************************************************************/

#include "unitree_legged_sdk/unitree_legged_sdk.h"
#include <math.h>
#include <iostream>
#include <unistd.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024
#define PORT 1145

using namespace UNITREE_LEGGED_SDK;

class Custom
{
public:
    Custom(uint8_t level) : safe(LeggedType::Go1),
                            udp(level, 8090, "192.168.123.161", 8082)
    {
        udp.InitCmdData(cmd);
    }
    void UDPRecv();
    void UDPSend();
    void UDPLink();
    void UDPMatlab();

    Safety safe;
    UDP udp;
    HighCmd cmd = {0};
    HighState state = {0};
    int motiontime = 0;
    float dt = 0.002; // 0.001~0.01

    float data[11] = {0};
    int server_fd, n, i = 0;
    char buffer[MAX_BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
};

void Custom::UDPRecv()
{
    udp.Recv();
}

void Custom::UDPSend()
{
    udp.Send();
}

void Custom::UDPMatlab()
{

    if (i == 0) 
    {

        server_fd = socket(AF_INET, SOCK_DGRAM, 0);
        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(PORT);

        bind(server_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr));

        len = sizeof(cliaddr);

        i = 1;
    }
    recvfrom(server_fd, data, sizeof(data), 0, NULL, NULL);

    //printf("Received Matlab data: %f, %f\n", data[0], data[1]);
}

void Custom::UDPLink()
{

    motiontime += 2;
    udp.GetRecv(state);
    // printf("%d   %f\n", motiontime, state.imu.quaternion[2]);

    cmd.mode = static_cast<uint8_t>(data[0]);
    /*
    uint8_t
    0. idle, default stand
    1. force stand (controlled by dBodyHeight + ypr)
    2. target velocity walking (controlled by velocity + yawSpeed)
    3. target position walking (controlled by position + ypr[0])
    4. path mode walking (reserve for future release)
    5. position stand down.
    6. position stand up
    7. damping mode
    8. recovery stand
    9. backflip
    10. jumpYaw
    11. straightHand
    12. dance1
    13. dance2
    */
    cmd.gaitType = static_cast<uint8_t>(data[1]);
    /*
    uint8_t
    0.idle
    1.trot
    2.trot running
    3.climb stair
    4.trot obstacle
    */
    cmd.speedLevel = static_cast<uint8_t>(data[2]);
    /*
    uint8_t
    0. default low speed.
    1. medium speed
    2. high speed. during walking, only respond MODE 3
    */
    cmd.footRaiseHeight = data[3]; // float (unit: m, default: 0.08m), foot up height while walking, delta value
    cmd.bodyHeight = data[4];     // float (unit: m, default: 0.28m), delta value
    cmd.euler[0] = data[5]; // float (unit: rad), roll pitch yaw in stand mode
    cmd.euler[1] = data[6];
    cmd.euler[2] = data[7];
    cmd.velocity[0] = data[8]; // float (unit: m/s), forwardSpeed,  in body frame
    cmd.velocity[1] = data[9]; // float (unit: m/s), sideSpeed
    cmd.yawSpeed = data[10];    // float (unit: rad/s), rotateSpeed in body frame
    cmd.reserve = 0;

    printf("mode:%d\ngaitType:%d\nspeedLevel:%d\n", cmd.mode,cmd.gaitType,cmd.speedLevel);

    udp.SetSend(cmd);
}

int main(void)
{
    std::cout << "Communication level is set to HIGH-level." << std::endl
              << "WARNING: Make sure the robot is standing on the ground." << std::endl
              << "Press Enter to continue..." << std::endl;
    std::cin.ignore();

    Custom custom(HIGHLEVEL);
    // InitEnvironment();
    LoopFunc loop_udpMatlab("udp_matlab", custom.dt, boost::bind(&Custom::UDPMatlab, &custom));
    LoopFunc loop_udpLink("udp_link", custom.dt, boost::bind(&Custom::UDPLink, &custom));
    LoopFunc loop_udpSend("udp_send", custom.dt, 3, boost::bind(&Custom::UDPSend, &custom));
    LoopFunc loop_udpRecv("udp_recv", custom.dt, 3, boost::bind(&Custom::UDPRecv, &custom));

    loop_udpMatlab.start();
    loop_udpSend.start();
    loop_udpRecv.start();
    loop_udpLink.start();

    while (1)
    {
        sleep(10);

        // i++;
    };

    return 0;
}
