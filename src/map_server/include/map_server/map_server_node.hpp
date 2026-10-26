#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

namespace map_server
{

class MapServerNode : public rclcpp::Node
{
public:
  explicit MapServerNode(const rclcpp::NodeOptions & options);

private:
  void publish_map();

  sensor_msgs::msg::PointCloud2 map_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr map_publisher_;
  rclcpp::TimerBase::SharedPtr publish_timer_;
};

}  // namespace map_server
