#pragma once

#include <memory>
#include <mutex>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <tf2_ros/static_transform_broadcaster.hpp>

namespace map_server
{

class MapServerNode : public rclcpp::Node
{
public:
  explicit MapServerNode(const rclcpp::NodeOptions & options);

private:
  void publish_map();
  void update_pcd_offset();

  sensor_msgs::msg::PointCloud2 map_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr map_publisher_;
  geometry_msgs::msg::TransformStamped pcd_transform_;
  std::unique_ptr<tf2_ros::StaticTransformBroadcaster> pcd_transform_broadcaster_;
  nav_msgs::msg::OccupancyGrid occupancy_map_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr occupancy_publisher_;
  std::mutex map_mutex_;
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_validation_;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr parameter_update_;
  rclcpp::TimerBase::SharedPtr publish_timer_;
};

}  // namespace map_server
