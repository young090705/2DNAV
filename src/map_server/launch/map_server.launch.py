from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import ComposableNodeContainer
from launch_ros.descriptions import ComposableNode


def generate_launch_description():
    share = Path(get_package_share_directory('map_server'))
    config_file = share / 'config' / 'params.yaml'
    return LaunchDescription([
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
        )
    ])
