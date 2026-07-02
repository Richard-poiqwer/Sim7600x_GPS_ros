#include "sim7600gps/usbCom.h"
#include <iostream>
#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class MinimalPublisher : public rclcpp::Node
{
public:

    UsbCom usb;

    MinimalPublisher()
    : Node("minimal_publisher")
    {
        this->usb = usb("/dev/ttyUSB2", 115200);

        while (this->usb.openPort()) {
          continue;
        }
        while (this->usb.sendAT())
        {
          continue;
        }
        while (this->usb.GPSOn())
        {
          continue;
        }

        this->publisher_ = this->create_publisher<std_msgs::msg::String>("topic", 10);
        this->timer_ = this->create_wall_timer(0.5, this->timer_callback);
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

              float lat = fields[0]; // Derives lattitude number
              float lon = fields[1]; // Derives longitude number
              float altitude = fields[2]; // Derives Altitude
            }

			auto navMsg = std_msgs::msg::NavSatFix(
				header(),
				status(),
				latitude(lat),
				longitude(log),
				altitude(altitude),
				position_covariance(), // To be calclated manually?
				position_covariance_type(0) // UNKNOWN
			);								// CHANGE

			RCLCPP_INFO_STREAM(this->get_logger(), "Publishing: '" << navMsg.header << "'");
			publisher_->publish(navMsg);        
    };

private:
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
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
