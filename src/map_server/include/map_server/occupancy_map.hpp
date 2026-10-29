#pragma once

#include <filesystem>
#include <nav_msgs/msg/occupancy_grid.hpp>

namespace map_server
{
nav_msgs::msg::OccupancyGrid load_occupancy_map(const std::filesystem::path & yaml_path);
}  // namespace map_server
