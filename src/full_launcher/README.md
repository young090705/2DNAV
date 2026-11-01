# full_launcher

所有节点均直接定义在唯一的 `launch/full.launch.py` 中，统一启动 MID-360 驱动、FAST-LIO、地图发布器和可选 RViz。配置集中在本包 `config/`，无需修改第三方 launch。

## 编译与启动

在工作空间根目录执行：

```bash
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-up-to full_launcher
source install/setup.bash
ros2 launch full_launcher full.launch.py
```

默认启动三个节点，使用系统时间，不打开 RViz。打开地图可视化：

```bash
ros2 launch full_launcher full.launch.py use_rviz:=true
```

录包回放时关闭实时驱动，并为所有启动的节点启用仿真时间：

```bash
ros2 launch full_launcher full.launch.py use_sim_time:=true start_livox:=false
# 在另一个已加载 ROS 环境的终端播放录包，并发布 /clock：
ros2 bag play <bag_path> --clock
```

`use_sim_time` 控制节点的 ROS 时钟；传感器消息自身携带的硬件时间戳不会因此被重写。启用仿真时间时需要有 `/clock` 发布者。

## 节点开关与配置

| 节点 | 启动开关 | 参数 YAML |
| --- | --- | --- |
| MID-360 驱动 | `start_livox` | `config/livox.yaml` |
| FAST-LIO | `start_fast_lio` | `config/fast_lio.yaml` |
| PCD / PGM 地图发布器 | `start_map_server` | `config/map_server.yaml` |

三个节点开关默认均为 `true`。只启动地图发布器，或指定自定义地图配置：

```bash
ros2 launch full_launcher full.launch.py start_livox:=false start_fast_lio:=false
ros2 launch full_launcher full.launch.py map_server_params_file:=/absolute/path/to/map_server.yaml
```

使用 `livox_params_file`、`fast_lio_params_file`、`map_server_params_file` 可分别指定三个节点的配置路径。`use_sim_time` 统一传给所有节点（包括地图组件、容器和 RViz），并优先于 YAML 中的同名值。

MID-360 网络配置位于 `config/MID360_config.json`，沿用当前驱动配置：主机 IP `192.168.1.50`，雷达 IP `192.168.1.3`。按实际设备修改；也可修改 `livox.yaml` 的 `user_config_path` 使用其他 JSON。驱动的网络配置格式由 Livox SDK 决定，仍使用 JSON。

地图文件沿用 `map_server/maps/` 中的资源，保留 PCD 平移参数 `pcd_offset_x: 14.6`、`pcd_offset_y: 7.45`。FAST-LIO 参数沿用已有 MID-360 配置，包括外参与 PCD 保存设置；运行前按实际安装校准外参。

## 可视化坐标系

默认 RViz 配置显示 `map` 坐标系下的 PGM 和 PCD。FAST-LIO 输出使用 `camera_init` 坐标系，当前没有已标定的 `map` 到 `camera_init` 变换，本包不会自动添加该变换。查看 FAST-LIO 点云与轨迹时，可选用其原有 RViz 配置：

```bash
ros2 launch full_launcher full.launch.py use_rviz:=true \
  rviz_config:="$(ros2 pkg prefix fast_lio)/share/fast_lio/rviz/fastlio.rviz"
```

MID-360 与 FAST-LIO 默认配置参考仓库内的 `livox_ros_driver2` 和 `fast_lio` 对应配置，相关第三方内容仍遵循原项目许可证。
