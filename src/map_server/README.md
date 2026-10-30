# 地图加载与显示

C++ ROS 2 组件 `map_server::MapServerNode` 支持 PCD 点云和 PGM/YAML 二维地图。
加载 `config/params.yaml` 中选择的文件，RViz 可同时显示二维地图和点云。

```bash
cd /home/rm-young/2DNav
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-select map_server
source install/setup.bash
ros2 launch map_server map_server.launch.py use_rviz:=true
```

无界面运行：`ros2 launch map_server map_server.launch.py use_rviz:=false`。

依赖包含 ROS 2 Jazzy、PCL、yaml-cpp 和 OpenCV。
