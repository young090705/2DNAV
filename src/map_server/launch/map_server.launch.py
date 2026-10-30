from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import ComposableNodeContainer, Node
from launch_ros.descriptions import ComposableNode


def generate_launch_description():
    share = Path(get_package_share_directory('map_server'))
    config_file = share / 'config' / 'params.yaml'
    return LaunchDescription([
        DeclareLaunchArgument('use_rviz', default_value='false',
                              description='Open RViz2 with the PGM map display'),
        ComposableNodeContainer(
            name='map_server_container',
            namespace='',
            package='rclcpp_components',
            executable='component_container',
            output='screen',
            composable_node_descriptions=[
                ComposableNode(
                    package='map_server',
                    plugin='map_server::MapServerNode',
                    name='map_server',
                    parameters=[str(config_file)],
                ),
            ],
        ),
        Node(
            package='rviz2', executable='rviz2', name='map_rviz',
            arguments=['-d', str(share / 'config' / 'map.rviz')],
            condition=IfCondition(LaunchConfiguration('use_rviz')),
            output='screen',
        ),
    ])
