cmake_minimum_required(VERSION 3.16)
project(map_server)

if(POLICY CMP0144)
  cmake_policy(SET CMP0144 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgcodecs)
find_package(pcl_conversions REQUIRED)
find_package(PCL REQUIRED COMPONENTS io)

add_library(${PROJECT_NAME} SHARED src/map_server_node.cpp src/occupancy_map.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${PCL_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${PCL_LIBRARIES} yaml-cpp ${OpenCV_LIBS})
ament_target_dependencies(${PROJECT_NAME}
  ament_index_cpp rclcpp rclcpp_components sensor_msgs nav_msgs pcl_conversions)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)

# Register the shared library only; the ROS component container supplies main().
rclcpp_components_register_nodes(${PROJECT_NAME} "map_server::MapServerNode")

install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY config launch maps DESTINATION share/${PROJECT_NAME})



ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_dependencies(rclcpp sensor_msgs nav_msgs)
ament_package()
