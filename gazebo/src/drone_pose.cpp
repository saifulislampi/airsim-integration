/*
 * Copyright (C) 2012 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// Taken from the Microsoft AirSim repo: https://github.com/Microsoft/AirSim.git

#include <gazebo/transport/transport.hh>
#include <gazebo/msgs/msgs.hh>
#include <gazebo/gazebo_client.hh>
#include "common/common_utils/StrictMode.hpp"
STRICT_MODE_OFF
#ifndef RPCLIB_MSGPACK
#define RPCLIB_MSGPACK clmdep_msgpack
#endif // !RPCLIB_MSGPACK
#include "rpc/rpc_error.h"
STRICT_MODE_ON

#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"
#include "Eigen/Dense"
#include <chrono>

#include <iostream>

constexpr int NWIDTH = 7;
static constexpr int MESSAGE_THROTTLE = 100;

using namespace msr::airlib;

msr::airlib::MultirotorRpcLibClient client;
std::vector<std::string> vehicleList;

void cbLocalPose(ConstPosesStampedPtr& msg)
{
    std::cout << std::fixed;
    std::cout << std::setprecision(3);
    static int count = 0;

    if (count % MESSAGE_THROTTLE == 0) {
        std::cout << "The number of vehicles is: " << vehicleList.size();
        std::cout << "\n";

        std::cout << "The first vehicle is: " << vehicleList[0];
        std::cout << "\n" << std::endl;
    }

    for (int i = 0; i < msg->pose_size(); i++) {
        auto x = msg->pose(i).position().x();
        auto y = msg->pose(i).position().y();
        auto z = msg->pose(i).position().z();
        auto ow = msg->pose(i).orientation().w();
        auto ox = msg->pose(i).orientation().x();
        auto oy = msg->pose(i).orientation().y();
        auto oz = msg->pose(i).orientation().z();
        if (count % MESSAGE_THROTTLE == 0) {
            std::cout << "local (" << std::setw(2) << i << ") ";
            std::cout << std::left << std::setw(32) << msg->pose(i).name();
            std::cout << " x: " << std::right << std::setw(NWIDTH) << x;
            std::cout << " y: " << std::right << std::setw(NWIDTH) << y;
            std::cout << " z: " << std::right << std::setw(NWIDTH) << z;

            std::cout << " ow: " << std::right << std::setw(NWIDTH) << ow;
            std::cout << " ox: " << std::right << std::setw(NWIDTH) << ox;
            std::cout << " oy: " << std::right << std::setw(NWIDTH) << oy;
            std::cout << " oz: " << std::right << std::setw(NWIDTH) << oz;
            std::cout << std::endl;
        }
        if (i == 0) {
            msr::airlib::Vector3r p(x, -y, -z);
            msr::airlib::Quaternionr o(ow, ox, -oy, -oz);

            client.simSetVehiclePose(Pose(p, o), true);
        }
    }
    if (count % MESSAGE_THROTTLE == 0) {
        std::cout << std::endl;
    }

    ++count;
}


// TODO: add C doctring
static Eigen::Vector4f camDroneToGlobal(
    msr::airlib::Quaternionr camDroneOrientation,
    msr::airlib::Quaternionr droneOrientation
) {
    // TODO: need to remove pitch and roll from drone orientation since camera pitch and roll
    // are already in the world frame
    msr::airlib::Quaternionr cam_global_orientation = droneOrientation * camDroneOrientation;

    return Eigen::Vector4f(
        cam_global_orientation.w(),
        cam_global_orientation.x(),
        cam_global_orientation.y(),
        cam_global_orientation.z()
    );
}


void cbDroneAsCameraPose(ConstPosesStampedPtr& msg)
{
    std::cout << std::fixed;
    std::cout << std::setprecision(3);
    static int count = 0;

    // initialize with nanf
    msr::airlib::Quaternionr drone_world_o(std::nanf(""), std::nanf(""), std::nanf(""), std::nanf(""));
    msr::airlib::Quaternionr cam_drone_o(std::nanf(""), std::nanf(""), std::nanf(""), std::nanf(""));
    msr::airlib::Quaternionr cam_global_o(std::nanf(""), std::nanf(""), std::nanf(""), std::nanf(""));
    msr::airlib::Vector3r p(std::nanf(""), std::nanf(""), std::nanf(""));
    
    if (count % MESSAGE_THROTTLE == 0) {
        std::cout << "The number of vehicles is: " << vehicleList.size();
        std::cout << "\n";

        std::cout << "The first vehicle is: " << vehicleList[0];
        std::cout << "\n" << std::endl;
    }

    for (int i = 0; i < msg->pose_size(); i++) {
        auto x = msg->pose(i).position().x();
        auto y = msg->pose(i).position().y();
        auto z = msg->pose(i).position().z();
        auto ow = msg->pose(i).orientation().w();
        auto ox = msg->pose(i).orientation().x();
        auto oy = msg->pose(i).orientation().y();
        auto oz = msg->pose(i).orientation().z();
        if (count % MESSAGE_THROTTLE == 0) {
            std::cout << "local (" << std::setw(2) << i << ") ";
            std::cout << std::left << std::setw(32) << msg->pose(i).name();
            std::cout << " x: " << std::right << std::setw(NWIDTH) << x;
            std::cout << " y: " << std::right << std::setw(NWIDTH) << y;
            std::cout << " z: " << std::right << std::setw(NWIDTH) << z;

            std::cout << " ow: " << std::right << std::setw(NWIDTH) << ow;
            std::cout << " ox: " << std::right << std::setw(NWIDTH) << ox;
            std::cout << " oy: " << std::right << std::setw(NWIDTH) << oy;
            std::cout << " oz: " << std::right << std::setw(NWIDTH) << oz;
            std::cout << std::endl;
        }
        // update freq ~250 hz
        // set drone position from drone position
        // TODO: set at camera's center position, not drone's
        if (i == 0) {
            p = msr::airlib::Vector3r(x, -y, -z);
            drone_world_o = Eigen::Vector4f(ow, ox, -oy, -oz);
            if (count % MESSAGE_THROTTLE == 0) {
                std::cout << "Camera drone quaternion: " << cam_drone_o.vec() << std::endl;
                std::cout << "Drone world quaternion: " << drone_world_o.vec() << std::endl;
            }
        }
        // set drone attitude from camera attitude
        if (msg->pose(i).name() == "typhoon_h480::cgo3_camera_link") {
            // orientation of the camera in drone's reference frame
            // cam_drone_o(ow, ox, -oy, -oz);
            cam_drone_o = Eigen::Vector4f(ow, ox, -oy, -oz);
            if (count % MESSAGE_THROTTLE == 0) {
                std::cout << "Camera drone quaternion: " << cam_drone_o.vec() << std::endl;
                std::cout << "Drone world quaternion: " << drone_world_o.vec() << std::endl;
            }
        }

        cam_global_o = camDroneToGlobal(cam_drone_o, drone_world_o);
        // if (count % MESSAGE_THROTTLE == 0) {
        //     std::cout << "Cam global orientation:" << cam_global_o.vec() << std::endl;
        // }
        // TODO: loop through vehicles for multidrone sim
        client.simSetVehiclePose(Pose(p, drone_world_o), true, vehicleList[0]);
    }
    if (count % MESSAGE_THROTTLE == 0) {
        std::cout << std::endl;
    }

    ++count;
}


void cbGlobalPose(ConstPosesStampedPtr& msg)
{
    std::cout << std::fixed;
    std::cout << std::setprecision(4);
    static int count = 0;
    if (count % MESSAGE_THROTTLE) {
        ++count;
        return;
    }
    ++count;

    for (int i = 0; i < msg->pose_size(); i++) {
        std::cout << "global (" << i << ") ";
        std::cout << std::left << std::setw(32) << msg->pose(i).name();
        std::cout << " x: " << std::right << std::setfill(' ') << std::setw(NWIDTH) << msg->pose(i).position().x();
        std::cout << " y: " << std::right << std::setfill(' ') << std::setw(NWIDTH) << msg->pose(i).position().y();
        std::cout << " z: " << std::right << std::setfill(' ') << std::setw(NWIDTH) << msg->pose(i).position().z();

        std::cout << " ow: " << std::right << std::setfill(' ') << std::setw(NWIDTH) << msg->pose(i).orientation().w();
        std::cout << " ox: " << std::right << std::setfill(' ') << std::setw(NWIDTH) << msg->pose(i).orientation().x();
        std::cout << " oy: " << std::right << std::setfill(' ') << std::setw(NWIDTH) << msg->pose(i).orientation().y();
        std::cout << " oz: " << std::right << std::setfill(' ') << std::setw(NWIDTH) << msg->pose(i).orientation().z();
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

int main(int argc, char** argv)
{
    void (*localPoseCallback)(ConstPosesStampedPtr&) = &cbLocalPose;
    for (int i=0; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            localPoseCallback = &cbDroneAsCameraPose;
        }
    }

    if (localPoseCallback == &cbDroneAsCameraPose) {
        std::cout << "Drone pose will be set to global gimbal camera pose\n";
    } else {
        std::cout << "Drone pose will be set to global drone pose\n";
    }

    client.confirmConnection();
    // don't want to call on every message because blocks for too long
    vehicleList = client.listVehicles();

    // Load gazebo
    gazebo::client::setup(argc, argv);

    // Create our node for communication
    gazebo::transport::NodePtr node(new gazebo::transport::Node());
    node->Init();

    // Listen to Gazebo topics
    // update freq ~250 hz
    gazebo::transport::SubscriberPtr sub_pose1 = node->Subscribe("~/pose/local/info", *localPoseCallback);
    // update freq ~50 hz
    gazebo::transport::SubscriberPtr sub_pose2 = node->Subscribe("~/pose/info", cbGlobalPose);

    while (true)
        gazebo::common::Time::MSleep(10);

    // Make sure to shut everything down.
    gazebo::client::shutdown();
}
