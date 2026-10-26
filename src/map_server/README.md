# 编译与启动

在工作区根目录运行：

```bash
cd /home/rm-young/2DNav
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-select map_server
source install/setup.bash
ros2 launch map_server map_server.launch.py
```



