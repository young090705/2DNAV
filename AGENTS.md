# 仓库协作规范

## ROS 2 launch 时间参数

- 每次为新节点编写 launch 文件时，必须提供 `use_sim_time` 启动参数，默认值为 `false`。
- 必须将该启动参数作为布尔类型的 ROS 参数传入启动的节点；仅声明启动参数而不传给节点不满足要求。
- 同一 launch 中启动的普通节点、组件节点和 RViz 等节点，应统一使用该参数。包含子 launch 时，应向支持该参数的子 launch 显式传递 `use_sim_time`。
- `use_sim_time:=false` 使用系统时间；`use_sim_time:=true` 使用 `/clock` 提供的仿真时间，可用于仿真或带时钟发布的录包回放。节点应使用 ROS 时钟获取时间，避免绕过该参数直接使用系统时钟。
- Python launch 中使用 `DeclareLaunchArgument` 声明参数，使用 `LaunchConfiguration` 获取参数，并通过 `ParameterValue(..., value_type=bool)` 传入节点的 `parameters`。
- 使用方式应支持：`ros2 launch <package> <file>.launch.py use_sim_time:=true`，也可传入 `false` 切换回系统时间。

