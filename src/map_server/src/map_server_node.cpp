#include "map_server/map_server_node.hpp"
#include "map_server/occupancy_map.hpp"

#include <chrono>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <pcl/PCLPointCloud2.h>
#include <pcl/io/pcd_io.h>
#include <pcl_conversions/pcl_conversions.h>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include "rclcpp_components/register_node_macro.hpp"

namespace map_server
{

MapServerNode::MapServerNode(const rclcpp::NodeOptions & options)
: Node("map_server", options)
{
  const auto pcd_file = declare_parameter<std::string>("pcd_file", "");
  rcl_interfaces::msg::ParameterDescriptor frame_descriptor;
  frame_descriptor.read_only = true;
  frame_descriptor.description = "PCD child frame; applied at startup";
  const auto pcd_frame_id = declare_parameter<std::string>(
    "pcd_frame_id", "pcd_map", frame_descriptor);
  const auto map_yaml = declare_parameter<std::string>("map_yaml", "");
  const auto map_topic = declare_parameter<std::string>("map_topic", "/map");
  const auto frame_id = declare_parameter<std::string>("frame_id", "map");
  const auto topic_name = declare_parameter<std::string>("topic_name", "/map_server/map_cloud");
  const auto publish_rate = declare_parameter<double>("publish_rate", 1.0);

  //安全检查
  if (pcd_file.empty() && map_yaml.empty()) {
    throw std::invalid_argument("Specify pcd_file and/or map_yaml");
  }
  if (frame_id.empty() || (!pcd_file.empty() && topic_name.empty()) ||
    (!map_yaml.empty() && map_topic.empty()))
  {
    throw std::invalid_argument("frame_id and enabled map topics must not be empty");
  }
  if (!pcd_file.empty() && !map_yaml.empty() &&
    get_node_topics_interface()->resolve_topic_name(topic_name) ==
    get_node_topics_interface()->resolve_topic_name(map_topic))
  {
    throw std::invalid_argument("PCD and occupancy map topics must be distinct");
  }
  if (!pcd_file.empty() && (pcd_frame_id.empty() || pcd_frame_id == frame_id ||
    pcd_frame_id.front() == '/' || frame_id.front() == '/'))
  {
    throw std::invalid_argument("PCD and parent frames must be distinct, without leading slashes");
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

  if (!pcd_file.empty()) {
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

    //检查点云是否有xyz三个字段的坐标
    for (const std::string axis : {"x", "y", "z"}) {
      bool found = false;
      for (const auto & field : cloud.fields) {
        found = found || field.name == axis;
      }
      if (!found) {
        throw std::runtime_error("PCD map is missing coordinate field: " + axis);
      }
    }
    pcl_conversions::moveFromPCL(cloud, map_); //将PCL点云格式转化为ROS2点云消息
    map_.header.frame_id = pcd_frame_id;
    pcd_transform_.header.frame_id = frame_id;
    pcd_transform_.child_frame_id = pcd_frame_id;
    pcd_transform_.transform.rotation.w = 1.0;
    rcl_interfaces::msg::ParameterDescriptor offset_descriptor;
    offset_descriptor.description = "PCD translation along parent map X in meters (live)";
    declare_parameter<double>("pcd_offset_x", 0.0, offset_descriptor);
    offset_descriptor.description = "PCD translation along parent map Y in meters (live)";
    declare_parameter<double>("pcd_offset_y", 0.0, offset_descriptor);
    update_pcd_offset();
    pcd_transform_broadcaster_ = std::make_unique<tf2_ros::StaticTransformBroadcaster>(this);
    map_publisher_ = create_publisher<sensor_msgs::msg::PointCloud2>(
      topic_name, rclcpp::QoS(1).reliable().transient_local());
    const auto is_offset_parameter = [](const rclcpp::Parameter & parameter) {
        return parameter.get_name() == "pcd_offset_x" || parameter.get_name() == "pcd_offset_y";
      };
    parameter_validation_ = add_on_set_parameters_callback(
      [is_offset_parameter](const std::vector<rclcpp::Parameter> & parameters) {
        rcl_interfaces::msg::SetParametersResult result;
        result.successful = true;
        for (const auto & parameter : parameters) {
          if (is_offset_parameter(parameter) &&
            (parameter.get_type() != rclcpp::ParameterType::PARAMETER_DOUBLE ||
            !std::isfinite(parameter.as_double())))
          {
            result.successful = false;
            result.reason = parameter.get_name() + " must be a finite double";
            break;
          }
        }
        return result;
      });
    parameter_update_ = add_post_set_parameters_callback(
      [this, is_offset_parameter](const std::vector<rclcpp::Parameter> & parameters) {
        if (std::any_of(parameters.begin(), parameters.end(), is_offset_parameter)) {
          update_pcd_offset();
          publish_map();
        }
      });
  }

  if (!map_yaml.empty()) {
    std::filesystem::path path(map_yaml);
    if (path.is_relative()) {
      path = std::filesystem::path(
        ament_index_cpp::get_package_share_directory("map_server")) / "maps" / path;
    }
    occupancy_map_ = load_occupancy_map(path);
    occupancy_map_.header.frame_id = frame_id;
    occupancy_map_.info.map_load_time = now();
    occupancy_publisher_ = create_publisher<nav_msgs::msg::OccupancyGrid>(
      map_topic, rclcpp::QoS(1).reliable().transient_local());
  }

  publish_map();
  if (publish_rate > 0.0) {
    publish_timer_ = create_wall_timer(period, [this]() { publish_map(); });
  }

  RCLCPP_INFO(get_logger(), "MapServerNode Start!");
}

void MapServerNode::publish_map()
{
  std::lock_guard<std::mutex> lock(map_mutex_);
  const auto stamp = now();
  if (map_publisher_) {
    pcd_transform_.header.stamp = stamp;
    pcd_transform_broadcaster_->sendTransform(pcd_transform_);
    map_.header.stamp = stamp;
    map_publisher_->publish(map_);
  }
  if (occupancy_publisher_) {
    occupancy_map_.header.stamp = stamp;
    occupancy_publisher_->publish(occupancy_map_);
  }
}

void MapServerNode::update_pcd_offset()
{
  const auto x = get_parameter("pcd_offset_x").as_double();
  const auto y = get_parameter("pcd_offset_y").as_double();
  if (!std::isfinite(x) || !std::isfinite(y)) {
    throw std::invalid_argument("PCD offsets must be finite");
  }
  std::lock_guard<std::mutex> lock(map_mutex_);
  pcd_transform_.transform.translation.x = x;
  pcd_transform_.transform.translation.y = y;
}


}  // namespace map_server

RCLCPP_COMPONENTS_REGISTER_NODE(map_server::MapServerNode)
