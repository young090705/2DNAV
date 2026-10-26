colcon build --symlink-install --packages-select map_server
source install/setup.bash
ros2 launch map_server map_server.launch.py