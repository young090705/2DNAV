#include "map_server/map_server_node.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <pcl/PCLPointCloud2.h>
#include <pcl/io/pcd_io.h>
#include <pcl_conversions/pcl_conversions.h>
#include "rclcpp_components/register_node_macro.hpp"

namespace map_server
{

MapServerNode::MapServerNode(const rclcpp::NodeOptions & options)
: Node("map_server", options)
{
  const auto pcd_file = declare_parameter<std::string>("pcd_file", "");
  const auto frame_id = declare_parameter<std::string>("frame_id", "map");
  const auto topic_name = declare_parameter<std::string>("topic_name", "/map_server/map_cloud");
  const auto publish_rate = declare_parameter<double>("publish_rate", 1.0);

  if (pcd_file.empty()) {
    throw std::invalid_argument("pcd_file must specify a PCD map file");
  }
  if (frame_id.empty() || topic_name.empty()) {
    throw std::invalid_argument("frame_id and topic_name must not be empty");
  }
  if (!std::isfinite(publish_rate) || publish_rate < 0.0) {
    throw std::invalid_argument("publish_rate must be finite and >= 0 (0 publishes once)");
  }
  std::chrono::nanoseconds period{0};
  if (publish_rate > 0.0) {
    const long double period_ns = 1.0e9L / publish_rate;
    if (period_ns < 1.0L ||
      period_ns >= static_cast<long double>(std::numeric_limits<int64_t>::max()))
    {
      throw std::invalid_argument("publish_rate is outside the nanosecond timer range");
    }
    period = std::chrono::nanoseconds(static_cast<int64_t>(period_ns));
  }

  std::filesystem::path path(pcd_file);
  if (path.is_relative()) {
    path = std::filesystem::path(
      ament_index_cpp::get_package_share_directory("map_server")) / "maps" / path;
  }

  // Untyped PCLPointCloud2 retains intensity, RGB and other original PCD fields.
  pcl::PCLPointCloud2 cloud;
  if (pcl::io::loadPCDFile(path.string(), cloud) < 0) {
    throw std::runtime_error("Failed to load PCD map: " + path.string());
  }
  if (cloud.width == 0 || cloud.height == 0 || cloud.data.empty()) {
    throw std::runtime_error("PCD map contains no points: " + path.string());
  }
  for (const std::string axis : {"x", "y", "z"}) {
    bool found = false;
    for (const auto & field : cloud.fields) {
      found = found || field.name == axis;
    }
    if (!found) {
      throw std::runtime_error("PCD map is missing coordinate field: " + axis);
    }
  }
  pcl_conversions::moveFromPCL(cloud, map_);
  map_.header.frame_id = frame_id;
  map_publisher_ = create_publisher<sensor_msgs::msg::PointCloud2>(
    topic_name, rclcpp::QoS(1).reliable().transient_local());

  publish_map();
  if (publish_rate > 0.0) {
    publish_timer_ = create_wall_timer(period, [this]() { publish_map(); });
  }
  RCLCPP_INFO(
    get_logger(), "Loaded %u x %u points from %s; topic=%s, frame=%s",
    map_.width, map_.height, path.string().c_str(),
    map_publisher_->get_topic_name(), frame_id.c_str());
}

void MapServerNode::publish_map()
{
  map_.header.stamp = now();
  map_publisher_->publish(map_);
}

}  // namespace map_server

RCLCPP_COMPONENTS_REGISTER_NODE(map_server::MapServerNode)
