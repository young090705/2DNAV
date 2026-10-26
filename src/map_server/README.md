# PCD 地图组件

ROS 2 Jazzy 的独立 `map_server` 包。`map_server::MapServerNode` 继承
`rclcpp::Node`，使用 `RCLCPP_COMPONENTS_REGISTER_NODE` 注册为共享库组件；
没有自定义 `main()`，由 `rclcpp_components/component_container` 承载。
仅负责读取 PCD 并发布三维地图，不生成二维 OccupancyGrid。

## 编译与启动

在工作区根目录运行：

```bash
cd /home/rm-young/2DNav
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-select map_server
source install/setup.bash
ros2 launch map_server map_server.launch.py \
  pcd_file:=/home/rm-young/hwsentry_ws/HWSentryNav26/map_server/maps/RMUC202608af.pcd
```

默认同时启动容器和 RViz2，加载预设 PointCloud2 显示，按 Z 高度着色。
无桌面环境时加 `rviz:=false`。如果地图远离原点，在 RViz2 中选中地图并按 `F`
聚焦，或调整 Views 的 Focal Point / Distance。

也可以把地图放入本包 `maps/`，重新编译后传 `pcd_file:=your_map.pcd`。
绝对路径直接读取，相对路径以本包安装后的 `share/map_server/maps/` 为基准。
支持 PCL 可读取的 ASCII、binary、binary_compressed PCD；保留 XYZ、intensity、RGB
等原始字段。文件缺失、空点云或缺少 XYZ 字段会报错并使组件加载失败。

## 参数

| 参数 | 默认值 | 含义 |
| --- | --- | --- |
| `pcd_file` | 空，启动时必须指定 | PCD 文件路径 |
| `frame_id` | `map` | 点云坐标系；只设置标头，不变换坐标 |
| `topic_name` | `/map_server/map_cloud` | PointCloud2 发布话题 |
| `publish_rate` | `1.0` | 发布频率 Hz；0 表示只发布一次 |
| `use_sim_time` | `false` | 使用仿真时钟 |

以上参数启动时生效，不支持运行中重新加载地图。发布 QoS 为 Reliable +
Transient Local + Keep Last(1)，晚加入的订阅者也能获取地图。
RViz2 预设使用相同 QoS；自定义订阅者建议也启用 Transient Local。
使用其他 `frame_id` 或 `topic_name` 时，launch 会同步设置 RViz2。

## 加载到已有容器

```bash
ros2 run rclcpp_components component_container --ros-args -r __node:=map_container
# 另一个终端 source 相同环境后：
ros2 component load /map_container map_server map_server::MapServerNode \
  -p pcd_file:=/absolute/path/to/map.pcd -p frame_id:=map
```

已有系统中若也有名为 `map_server` 的包，应确认当前 overlay 解析到本包：
`ros2 pkg prefix map_server`。

## 验证

```bash
colcon test --packages-select map_server --event-handlers console_direct+
colcon test-result --verbose
ros2 component types | grep -A 1 map_server
ros2 topic info /map_server/map_cloud --verbose
```

测试覆盖真实 PCD 的字段与数值、晚订阅接收、缺失文件、空路径及非法频率。

## 文件结构

```text
map_server/
├── CMakeLists.txt
├── package.xml
├── include/map_server/map_server_node.hpp
├── src/map_server_node.cpp
├── config/params.yaml
├── launch/map_server.launch.py
├── rviz/map.rviz
├── maps/
└── test/
```
