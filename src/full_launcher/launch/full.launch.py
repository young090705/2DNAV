from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import ComposableNodeContainer, Node
from launch_ros.descriptions import ComposableNode
from launch_ros.parameter_descriptions import ParameterFile, ParameterValue


def generate_launch_description():
    share = Path(get_package_share_directory('full_launcher'))
    map_share = Path(get_package_share_directory('map_server'))
    use_sim_time = ParameterValue(LaunchConfiguration('use_sim_time'), value_type=bool)

    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='false',
                              description='Use the /clock ROS simulation clock for all nodes'),
        DeclareLaunchArgument('start_livox', default_value='true',
                              description='Start the live MID-360 driver; disable for bag replay'),
        DeclareLaunchArgument('start_fast_lio', default_value='true',
                              description='Start FAST-LIO odometry and mapping'),
        DeclareLaunchArgument('start_map_server', default_value='true',
                              description='Publish the saved PCD and occupancy maps'),
        DeclareLaunchArgument('use_rviz', default_value='false',
                              description='Open one RViz2 instance'),
        DeclareLaunchArgument('rviz_config',
                              default_value=str(map_share / 'config' / 'map.rviz'),
                              description='RViz display configuration'),
        DeclareLaunchArgument('livox_params_file',
                              default_value=str(share / 'config' / 'livox.yaml'),
                              description='MID-360 driver ROS parameter YAML'),
        DeclareLaunchArgument('fast_lio_params_file',
                              default_value=str(share / 'config' / 'fast_lio.yaml'),
                              description='FAST-LIO MID-360 parameter YAML'),
        DeclareLaunchArgument('map_server_params_file',
                              default_value=str(share / 'config' / 'map_server.yaml'),
                              description='PCD and occupancy map parameter YAML'),
        Node(
            package='livox_ros_driver2',
            executable='livox_ros_driver2_node',
            name='livox_lidar_publisher',
            parameters=[
                ParameterFile(LaunchConfiguration('livox_params_file'), allow_substs=True),
                {'use_sim_time': use_sim_time},
            ],
            condition=IfCondition(LaunchConfiguration('start_livox')),
            output='screen',
        ),
        Node(
            package='fast_lio',
            executable='fastlio_mapping',
            name='laser_mapping',
            parameters=[LaunchConfiguration('fast_lio_params_file'),
                        {'use_sim_time': use_sim_time}],
            condition=IfCondition(LaunchConfiguration('start_fast_lio')),
            output='screen',
        ),
        ComposableNodeContainer(
            name='map_server_container',
            namespace='',
            package='rclcpp_components',
            executable='component_container',
            parameters=[{'use_sim_time': use_sim_time}],
            composable_node_descriptions=[
                ComposableNode(
                    package='map_server',
                    plugin='map_server::MapServerNode',
                    name='map_server',
                    parameters=[LaunchConfiguration('map_server_params_file'),
                                {'use_sim_time': use_sim_time}],
                ),
            ],
            condition=IfCondition(LaunchConfiguration('start_map_server')),
            output='screen',
        ),
        Node(
            package='rviz2',
            executable='rviz2',
            name='full_rviz',
            arguments=['-d', LaunchConfiguration('rviz_config')],
            parameters=[{'use_sim_time': use_sim_time}],
            condition=IfCondition(LaunchConfiguration('use_rviz')),
            output='screen',
        ),
    ])
