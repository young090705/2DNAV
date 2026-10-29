#include "map_server/occupancy_map.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <opencv2/imgcodecs.hpp>
#include <yaml-cpp/yaml.h>

namespace map_server
{
nav_msgs::msg::OccupancyGrid load_occupancy_map(const std::filesystem::path & yaml_path)
{
  try {
    const auto config = YAML::LoadFile(yaml_path.string());
    if (config["mode"].as<std::string>("trinary") != "trinary") {
      throw std::runtime_error("Only trinary map mode is supported");
    }
    const auto resolution = config["resolution"].as<double>();
    const auto occupied = config["occupied_thresh"].as<double>();
    const auto free = config["free_thresh"].as<double>();
    const auto negate = config["negate"].as<int>();
    const auto origin = config["origin"].as<std::vector<double>>();
    if (!std::isfinite(resolution) || resolution <= 0.0 ||
      resolution > std::numeric_limits<float>::max() ||
      static_cast<float>(resolution) == 0.0f)
    {
      throw std::runtime_error("resolution must be finite, positive and fit in float32");
    }
    if (!std::isfinite(free) || !std::isfinite(occupied) ||
      free < 0.0 || occupied > 1.0 || free >= occupied)
    {
      throw std::runtime_error("Require 0 <= free_thresh < occupied_thresh <= 1");
    }
    if (negate != 0 && negate != 1) {
      throw std::runtime_error("negate must be 0 or 1");
    }
    if (origin.size() != 3 || !std::isfinite(origin[0]) ||
      !std::isfinite(origin[1]) || !std::isfinite(origin[2]))
    {
      throw std::runtime_error("origin must contain finite [x, y, yaw]");
    }
    std::filesystem::path image_path(config["image"].as<std::string>());
    if (image_path.empty()) {throw std::runtime_error("image must not be empty");}
    if (image_path.is_relative()) {image_path = yaml_path.parent_path() / image_path;}
    if (!std::filesystem::is_regular_file(image_path)) {
      throw std::runtime_error("Image file does not exist: " + image_path.string());
    }
    const auto image = cv::imread(image_path.string(), cv::IMREAD_UNCHANGED);
    if (image.empty() || image.type() != CV_8UC1) {
      throw std::runtime_error("Expected an 8-bit grayscale map: " + image_path.string());
    }

    nav_msgs::msg::OccupancyGrid map;
    map.info.width = static_cast<uint32_t>(image.cols);
    map.info.height = static_cast<uint32_t>(image.rows);
    map.info.resolution = static_cast<float>(resolution);
    map.info.origin.position.x = origin[0];
    map.info.origin.position.y = origin[1];
    map.info.origin.orientation.z = std::sin(origin[2] / 2.0);
    map.info.origin.orientation.w = std::cos(origin[2] / 2.0);
    map.data.resize(static_cast<size_t>(image.cols) * image.rows);
    // PGM starts at the top left; OccupancyGrid starts at the bottom left.
    for (int y = 0; y < image.rows; ++y) {
      const auto * row = image.ptr<uint8_t>(image.rows - 1 - y);
      for (int x = 0; x < image.cols; ++x) {
        const double shade = row[x] / 255.0;
        const double probability = negate ? shade : 1.0 - shade;
        map.data[static_cast<size_t>(y) * image.cols + x] =
          probability > occupied ? 100 : (probability < free ? 0 : -1);
      }
    }
    return map;
  } catch (const std::exception & error) {
    throw std::runtime_error("Failed to load map " + yaml_path.string() + ": " + error.what());
  }
}
}  // namespace map_server
