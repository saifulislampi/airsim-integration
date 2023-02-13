#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include "pose.hpp"

#ifndef UDP_RECEIVER_H
#define UDP_RECEIVER_H


class UDPReceiver {       // The class
    public:             // Access specifier
        UDPReceiver(unsigned short int listener_port);  // Constructor
        PoseTransfer::PoseMessage listen_pose_message();  // Method/function declaration
    private:
        static const unsigned long decimal_offset = 1e5; // used to convert doubles to uint64_t
        unsigned short int listener_port;
        int sock;
        #pragma pack(push,1)
        typedef struct UdpPose {
            uint64_t x;
            uint64_t y;
            uint64_t z;
            uint64_t w;
            uint64_t xi;
            uint64_t yj;
            uint64_t zk;
        } UdpPose;
        typedef struct UdpPoseMessage {
            uint64_t message_counter; // this is a counter that increments every time we send a message
            UdpPose drone;
            UdpPose camera;
        } UdpPoseMessage;
        #pragma pack(pop)
        PoseTransfer::PoseMessage udp_message_to_pose(UdpPoseMessage);
};

#endif