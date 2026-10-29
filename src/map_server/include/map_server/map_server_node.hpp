#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>

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
  nav_msgs::msg::OccupancyGrid occupancy_map_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr occupancy_publisher_;
  rclcpp::TimerBase::SharedPtr publish_timer_;
};

}  // namespace map_server
