# 地图加载与显示

C++ ROS 2 组件 `map_server::MapServerNode` 支持 PCD 点云和 PGM/YAML 二维地图。
默认同时加载 `maps/RMUC202608af.pcd` 和 `maps/RMUC2026.yaml`，RViz 默认显示二维 `/map`。

```bash
cd /home/rm-young/2DNav
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-select map_server
source install/setup.bash
ros2 launch map_server map_server.launch.py
```

无界面运行：`ros2 launch map_server map_server.launch.py use_rviz:=false`。

修改 `config/params.yaml` 选择地图：

| 参数 | 默认值 | 用途 |
| --- | --- | --- |
| `map_yaml` | `RMUC2026.yaml` | PGM 地图 YAML，置空关闭二维地图 |
| `map_topic` | `/map` | `nav_msgs/msg/OccupancyGrid` 话题 |
| `pcd_file` | `RMUC202608af.pcd` | PCD 地图，置空关闭点云 |
| `topic_name` | `/map_server/map_cloud` | `sensor_msgs/msg/PointCloud2` 话题 |
| `frame_id` | `map` | 两种地图的坐标系 |
| `publish_rate` | `1.0` | Hz；0 表示只发布一次 |

至少启用一种地图。地图路径支持绝对路径；相对路径基于已安装包的 `maps/` 目录。
YAML 的 `image` 相对路径基于 YAML 所在目录。
两种消息使用 Reliable + Transient Local QoS，晚启动的订阅者也能收到地图。

二维地图由 `src/occupancy_map.cpp` 使用 yaml-cpp 和 OpenCV 直接读取，支持 8 位灰度
PGM（P2/P5）和 `trinary` 模式，解析 `resolution`、`origin: [x, y, yaw]`、`negate`、
`occupied_thresh`、`free_thresh`。像素转为 100（占据）、0（空闲）、-1（未知），
并将图像行翻转为 OccupancyGrid 的地图坐标。
RMUC2026 为 584×301、0.05 米/格，原点 `[-4.4, -6.96, 0]`。

RViz 配置在 `config/map.rviz`，可勾选 Optional PCD 查看点云。
两种地图按各自原始坐标显示，此节点不执行地图配准。
改变话题或 frame_id 时，也需修改 RViz 中对应的 Topic 或 Fixed Frame。

依赖包含 ROS 2 Jazzy、PCL、yaml-cpp 和 OpenCV。
