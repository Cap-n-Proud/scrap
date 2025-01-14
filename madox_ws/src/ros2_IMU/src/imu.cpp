/*
   The MIT License (MIT)
   Copyright (c) 2019 Techno Road Inc.
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */
#include <chrono>
#include <iostream>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_msgs/msg/tf_message.hpp>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h> // for use tf2::Quaternion
// #include <diagnostic_updater/diagnostic_updater.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include "std_msgs/msg/string.hpp"
#include "RTIMULib.h"

// https://roboticsbackend.com/write-minimal-ros2-cpp-node/
using namespace std::chrono_literals;

class ImuNode : public rclcpp::Node {
public:

  explicit ImuNode(const rclcpp::NodeOptions& op)
    : Node("imu", op) {
    RCLCPP_INFO(this->get_logger(), "Inizializing");

    int x = init_imu();

    imu_pub = this->create_publisher < sensor_msgs::Imu("imu_topic", 10);
  }

  -ImuNode() {
    RCLCPP_INFO(this->get_logger(), "called destructor!");

    // imu_.Close();
  }

private:

  RTIMUSettings *settings = new RTIMUSettings("RTIMULib");
  RTIMU *imu              = RTIMU::createIMU(settings);
  int period_ms           = 50;

  // rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  size_t count_;
  PubImuData();

  int init_imu() {
    int sampleCount = 0;
    int sampleRate  = 0;
    uint64_t rateTimer;
    uint64_t displayTimer;
    uint64_t now;

    //  Using RTIMULib here allows it to use the .ini file generated by
    // RTIMULibDemo.
    //  Or, you can create the .ini in some other directory by using:
    //      RTIMUSettings *settings = new RTIMUSettings("<directory path>",
    // "RTIMULib");
    //  where <directory path> is the path to where the .ini file is to be
    // loaded/saved

    if ((imu == NULL) || (imu->IMUType() == RTIMU_TYPE_NULL)) {
      printf("No IMU found\n");
      exit(1);
    }

    //  set up IMU
    imu->IMUInit();

    //  this is a convenient place to change fusion parameters
    imu->setSlerpPower(0.02);
    imu->setGyroEnable(true);
    imu->setAccelEnable(true);
    imu->setCompassEnable(true);

    //  set up for rate timer
    rateTimer = displayTimer = RTMath::currentUSecsSinceEpoch();

    return 0;
  }

  void PubImuData() {
    usleep(imu->IMUGetPollInterval() * 1000);

    // can also be thins?  ros::Rate loop_rate(1000 /
    // imu->IMUGetPollInterval());
    std::string frame_id;
    frame_id = "imu_link";

    // auto message = std_msgs::msg::String();
    // message.data = "Hello, world! " + std::to_string(count_++);
    // RCLCPP_INFO(this->get_logger(), "Publishing: '%s'",
    // message.data.c_str());
    // publisher_->publish(message);

    while (imu->IMURead()) {
      RTIMU_DATA imu_data = imu->getIMUData();
      printf("%s\n",
             RTMath::displayDegrees("", imu_data.fusionPose));
      fflush(stdout);

      rclcpp::Time timestamp = this->get_clock()->now();

      sensor_msgs::Imu imu_msg;

      // look here
      // https://github.com/Garfield753/roscpprtimulib/blob/master/src/roscpprtimulib/src/roscpprtimulib.cpp

      if (imu->IMURead())
      {
        // RTIMU_DATA imu_data = imu->getIMUData();
        imu_msg.header.stamp          = timestamp; // ros::Time::now();
        imu_msg.header.frame_id       = frame_id;
        imu_msg.orientation.x         = imu_data.fusionQPose.x();
        imu_msg.orientation.y         = imu_data.fusionQPose.y();
        imu_msg.orientation.z         = imu_data.fusionQPose.z();
        imu_msg.orientation.w         = imu_data.fusionQPose.scalar();
        imu_msg.angular_velocity.x    = imu_data.gyro.x();
        imu_msg.angular_velocity.y    = imu_data.gyro.y();
        imu_msg.angular_velocity.z    = imu_data.gyro.z();
        imu_msg.linear_acceleration.x = imu_data.accel.x() * 9.81;
        imu_msg.linear_acceleration.y = imu_data.accel.y() * 9.81;
        imu_msg.linear_acceleration.z = imu_data.accel.z() * 9.81;

        imu_pub.publish(imu_msg);
      }
    }
  }
}; // end of class


int main(int argc, char **argv) {
  rclcpp::init(argc, argv);

  rclcpp::NodeOptions options;
  options.allow_undeclared_parameters(true);
  options.automatically_declare_parameters_from_overrides(true);

  auto imu_node = std::make_shared<ImuNode>(options);

  rclcpp::spin(imu_node);
  rclcpp::shutdown();
  return 0;
}
