#include "sim7600gps/usbCom.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "sensor_msgs/msg/nav_sat_fix.hpp"

using namespace std::chrono_literals;

class MinimalPublisher : public rclcpp::Node
{
public:

    UsbCom usb = UsbCom(115200);

    MinimalPublisher()
    : Node("minimal_publisher")
    {
        // this->usb = UsbCom("/dev/ttyUSB2", 115200);

        while (!this->usb.openPort("/dev/ttyUSB2")) {
          continue;
        }
        while (!this->usb.sendAT())
        {
          continue;
        }
        while (!this->usb.GPSOn())
        {
          continue;
        }

        this->publisher_ = this->create_publisher<sensor_msgs::msg::NavSatFix>("topic", 10);
        this->timer_ = this->create_wall_timer(std::chrono::milliseconds(500), 
            std::bind(&MinimalPublisher::timer_callback, this));
    }; 

    void timer_callback() {
      std::string GPSAnswer = this->usb.GPSRead();
      if (GPSAnswer=="") 
      {
        RCLCPP_INFO(this->get_logger(), "Not able to return information");
      }
      
      else 
      {
        auto fields = this->usb.split(GPSAnswer, ',');

        float lat = std::stof(fields[0]); // Derives lattitude number
        float longi = std::stof(fields[1]); // Derives longitude number
        float altitude = std::stof(fields[2]); // Derives Altitude
                                    //
        auto navMsg = sensor_msgs::msg::NavSatFix();

        navMsg.header.stamp = this->now();
        navMsg.header.frame_id = "gps_link";

        navMsg.latitude = lat;
        navMsg.longitude = longi;
        navMsg.altitude = altitude;

        navMsg.position_covariance = {
          1.0, 0.0, 0.0,
          0.0, 1.0, 0.0,
          0.0, 0.0, 1.0
        };

        navMsg.position_covariance_type = sensor_msgs::msg::NavSatFix::COVARIANCE_TYPE_DIAGONAL_KNOWN;

        // RCLCPP_INFO_STREAM(this->get_logger(), "Publishing: '" << navMsg.header << "'");
			  this->publisher_->publish(navMsg);        
      }


    };

private:
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<sensor_msgs::msg::NavSatFix>::SharedPtr publisher_;
    size_t count_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MinimalPublisher>());
    rclcpp::shutdown();
    return 0;

    // std::string GPSInfo = usb.GPSRead();
    // auto fields = usb.split(GPSInfo, ',');
}
