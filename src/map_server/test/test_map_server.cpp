#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include "map_server/map_server_node.hpp"

using namespace std::chrono_literals;

class MapServerTest : public ::testing::Test
{
protected:
  static void SetUpTestSuite() { rclcpp::init(0, nullptr); }
  static void TearDownTestSuite() { rclcpp::shutdown(); }

  rclcpp::NodeOptions options(const std::string & path, double rate = 0.0)
  {
    rclcpp::NodeOptions result;
    result.parameter_overrides({
      {"pcd_file", path}, {"frame_id", "test_map"},
      {"topic_name", "/test_map_cloud"}, {"publish_rate", rate}});
    return result;
  }
};

TEST_F(MapServerTest, LateSubscriberReceivesMapWithOriginalFields)
{
  // publish_rate=0: only the constructor publishes, before the subscriber exists.
  auto server = std::make_shared<map_server::MapServerNode>(options(TEST_PCD_PATH));
  auto listener = std::make_shared<rclcpp::Node>("map_test_listener");
  sensor_msgs::msg::PointCloud2::SharedPtr received;
  auto subscription = listener->create_subscription<sensor_msgs::msg::PointCloud2>(
    "/test_map_cloud", rclcpp::QoS(1).reliable().transient_local(),
    [&received](sensor_msgs::msg::PointCloud2::SharedPtr msg) { received = msg; });
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(server);
  executor.add_node(listener);
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while (!received && std::chrono::steady_clock::now() < deadline) {
    executor.spin_once(50ms);
  }
  ASSERT_NE(received, nullptr);
  EXPECT_EQ(received->header.frame_id, "test_map");
  EXPECT_EQ(received->width * received->height, 2u);
  EXPECT_GT(rclcpp::Time(received->header.stamp).nanoseconds(), 0);
  sensor_msgs::PointCloud2ConstIterator<float> x(*received, "x");
  sensor_msgs::PointCloud2ConstIterator<float> intensity(*received, "intensity");
  EXPECT_FLOAT_EQ(*x, 1.0f);
  EXPECT_FLOAT_EQ(*intensity, 42.0f);
  ++x;
  ++intensity;
  EXPECT_FLOAT_EQ(*x, 4.0f);
  EXPECT_FLOAT_EQ(*intensity, 84.0f);
}

TEST_F(MapServerTest, RejectsMissingFile)
{
  EXPECT_THROW(map_server::MapServerNode(options("/nonexistent/map.pcd")), std::runtime_error);
}

TEST_F(MapServerTest, RejectsEmptyPath)
{
  EXPECT_THROW(map_server::MapServerNode(options("")), std::invalid_argument);
}

TEST_F(MapServerTest, RejectsInvalidPublishRate)
{
  for (const double rate : {-1.0, 1.0e-300, 1.0e300, std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::quiet_NaN()})
  {
    EXPECT_THROW(map_server::MapServerNode(options(TEST_PCD_PATH, rate)), std::invalid_argument);
  }
}
